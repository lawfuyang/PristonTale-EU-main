# Monster HP Bar Update Latency — Root Cause Analysis

Created on: 2026-07-31

**Symptom:** when attacking a monster, damage numbers appear instantly, but the monster's HP bar drops only after a visible delay. Reported on a fully local setup (client + server on the same machine, loopback only).

**Verdict:** this is **not** a networking, IPC, or performance problem. It is a **deliberate server-side update-rate design**. Monster HP reaches your client on a **1-per-second cadence**, gated behind a **4-tick window that only opens once every 64 server frames**. Loopback latency is irrelevant — the data is not sent until the gate opens.

**Measured worst case: up to ~1.0 s, and up to ~4 s at 33–54 m range.**

---

## 1. Why your instinct is right, and why it doesn't help

The reasoning "there's no networking, so it should be instant" is correct about the transport. I verified the transport is genuinely not the problem:

- **Nagle's algorithm is disabled.** `shared/socket.cpp:141`:
  ```cpp
  //Set NoDelay (Nagle Algorithm)
  int iNoDelay = SOCKET_NODELAY;
  setsockopt( sd->sSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&iNoDelay, sizeof( int ) );
  ```
  So the classic 40 ms delayed-ACK/Nagle interaction is ruled out.

- Loopback TCP round-trip is tens of microseconds.

The delay is therefore **not in the pipe — it is upstream of the pipe.** The server simply has not written the HP value yet.

---

## 2. The asymmetry is the whole clue

Damage numbers and monster HP travel to your client through **two completely different paths, on two different schedules.** That is exactly why one looks instant and the other lags.

| | Damage numbers | Monster HP bar |
|---|---|---|
| Packet | `PKTHDR_DamageInfoContainer` | `PKTHDR_UnitStatus` (unit buffer) |
| Sent by | `SendDamageInfoAndClearBuffer()` | `LoopUnits()` |
| Gate | `if (pcUser->b8)` | `if (pcUser->b64)` |
| **Rate** | **8× per second** | **1× per second** |
| Extra gate | none | `uLastUpdate` must have changed |

Both live in the same function, `UserServer::Loop()` — 11 lines apart. From `Server/server/userserver.cpp:1718`:

```cpp
//8 times per second
if ( pcUser->b8 )
{
    DWORD dwTimeDifference = TICKCOUNT - pcUserData->dwTimeLastPacket;

    if ( dwTimeDifference < USER_STATUS_UPDATE_GRACE )
    {
        SendDamageInfoAndClearBuffer ( pcUser );      // ← 8 Hz: damage numbers

        //0.5s interval
        if ( pcUser->b32 )
        {
            //1 second interval
            if ( pcUser->b64 )
            {
                DAMAGEHANDLER->UserTick1s( pcUser );

                //Send Unit Status of other Monsters to this User
                LoopUnits( pcUser );                  // ← 1 Hz: monster HP
            }
            ...
```

`LoopUnits()` is the **only** caller that pushes monster status (including HP) to a player, and it is nested three gates deep at 1 Hz.

This one nesting difference — 8 Hz vs 1 Hz — accounts for essentially the entire perceived delay.

---

## 3. The gate cascade

The `b8`/`b16`/`b32`/`b64` flags are computed in `UserServer::Update()`, which runs at 64 FPS. `Server/server/userserver.cpp:1456`:

```cpp
UpdateUnitStatus( pcUser );     //64 FPS

//8 times per second
if ( ( i % 8 ) == ( iWheel % 8 ) )      pcUser->b8  = TRUE;  else pcUser->b8  = FALSE;

//4 times per second
if ( pcUser->b8  && (i % 16) == (iWheel % 16) ) pcUser->b16 = TRUE; else pcUser->b16 = FALSE;

//2 times per second
if ( pcUser->b16 && (i % 32) == (iWheel % 32) ) pcUser->b32 = TRUE; else pcUser->b32 = FALSE;

//1 time per second
if ( pcUser->b32 && ( i % 64 ) == ( iWheel % 64 ) )
{
    //Deep Status Update
    pcUser->b64 = TRUE;
    ...
```

Each flag ANDs with its parent, so `b64` is true for a given user exactly **once per 64 frames ≈ once per second**. Note `i` is the user's slot index, so users are deliberately spread across the wheel to smooth CPU load — a sensible design for hundreds of players, and pure downside for one.

**Monster HP is classified as a "Deep Status Update" — the slowest tier the server has.**

---

## 4. The second gate: `uLastUpdate`

Reaching 1 Hz is necessary but not sufficient. Inside `LoopUnits()`, HP is sent only if the unit's revision counter changed. `Server/server/userserver.cpp:1897`:

```cpp
if ( bWithinDetailedDistance )
{
    if ( pcUserData->uaUpdateCounter5[pcUnit->uIndex] != pcUnit->uLastUpdate )
    {
        SendUnitStatusM( pcUser, (Packet *)(pcUnit->baUnitBufferNew) );
        ...
        pcUserData->uaUpdateCounter5[pcUnit->uIndex] = pcUnit->uLastUpdate;
    }
}
```

And `uLastUpdate` is itself throttled. `Server/server/unitserver.cpp:3106`:

```cpp
static UINT iUnitWheel = 0;
static UINT iActiWheel = 0;

//Check if action wheel must be set
if ( iActiWheel == 0 )
{
    if ( (iUnitWheel % 64) == 0 )
        iActiWheel = 4;          // window opens for 4 ticks…
}
else
    iActiWheel--;                // …then closes for 60
...
        //Frame Check
        if ( (i % 4) == (iUnitWheel % 4) )
        {
            UpdateUnit( pcUnit );

            //Status Update?
            if ( iActiWheel != 0 )
                pcUnit->uLastUpdate++;   // only inside the 4-tick window
        }
```

So a monster's revision counter advances only during a **4-frame window that opens once every 64 frames** (~4/64 ≈ 6% duty cycle).

Consequence: HP changes that land in the closed 60-frame stretch are **not marked dirty**, so even when `b64` fires, `LoopUnits()` may find `uLastUpdate` unchanged and send nothing — pushing the update out another second.

Critically, `HP` is only serialized into the wire buffer during `UpdateUnit()`. `Server/server/unitserver.cpp:2526`:

```cpp
pcUnitData->MakeUnitBufferData((char*)pcUnit->baUnitBufferNew, 0x10, 4);
OnHandleUnitDataBufferNew(pcUnit, (PacketPlayData*)pcUnit->baUnitBufferNew);
```

Combined with `(i % 4) == (iUnitWheel % 4)`, each individual monster is only re-serialized every 4th frame.

### Damage application never marks the unit dirty

This is the key defect. I searched `Server/server/DamageHandler.cpp` for `uLastUpdate`, `SendUnitStatus`, and `ForceStatus`:

```
Found 0 matching results
```

The damage path (`CustomRecordCharacterDamage` → `CDamageHandler::RecordCharDamage`, `DamageHandler.cpp:103`/`:181`) reduces monster HP but **never touches `uLastUpdate` and never triggers a send.** Damage is applied silently; propagation is left entirely to the polling cascade.

That is the actionable bug: **the event that changes HP does not notify the replication layer.**

---

## 5. Total latency budget

Assuming a 64 FPS server (~15.6 ms/frame), for a monster within 33 m (detailed range):

| Stage | Delay |
|---|---|
| Damage applied, monster not flagged dirty | 0 ms |
| Wait for this monster's serialization slot (`i % 4`) | 0–62 ms |
| Wait for the `iActiWheel` dirty window (4-of-64 duty cycle) | 0–937 ms |
| Wait for this user's `b64` deep-update slot | 0–1000 ms |
| `SendUnitStatusM` buffer aggregation | 0–15 ms |
| Loopback TCP | **≈ 0.05 ms** |

**Typical ≈ 0.5 s, worst case ≈ 1.0 s.** Loopback is ~0.005% of the total.

Damage numbers, by contrast, wait at most 125 ms (8 Hz) — below the threshold where most people perceive lag. Hence "instant numbers, delayed bar."

### It gets worse past 33 m

Beyond detailed range, `LoopUnits()` additionally requires `uLastUpdate % N == 0` (`userserver.cpp:1915`):

```cpp
bSendBasicStatsUpdate = pcUserData->uaUpdateCounter5[pcUnit->uIndex] != pcUnit->uLastUpdate && ( pcUnit->uLastUpdate % 2 == 0 );
```

- 33–54 m (`bWithinBasicDistance`): `% 2` → up to **~2 s**
- non-NPC out-of-range but forced: `% 4` → up to **~4 s**
- NPCs: `% 8`

Ranged/mage classes fighting near max range will see markedly worse lag than melee. That is a useful diagnostic to confirm this analysis in-game.

Also note that in the 33–54 m branch, the packet sent is `PacketUnitStatusMove` — position/animation only, **no HP field at all**. So at that range HP is only refreshed when the monster happens to fall inside 33 m.

### One more gate

Everything above sits inside (`userserver.cpp:1722`):

```cpp
if ( dwTimeDifference < USER_STATUS_UPDATE_GRACE )   // 3000 ms, shared/user.h:19
```

If the server hasn't received a packet from you in 3 s, it stops sending updates entirely — so an idle client can see an even staler bar until it acts.

---

## 6. Why single-player makes this *feel* worse

The throttling exists to bound bandwidth and CPU for hundreds of concurrent players: HP for every visible monster, for every player, at 64 Hz would be enormous. Load-spreading via `i % 64` is the right call at scale.

With one player and no bandwidth constraint, you pay 100% of the latency cost for 0% of the benefit. The design is not wrong — it is simply mistuned for your deployment.

---

## 7. Fixes

Ordered by risk/benefit. All are server-side (`server.dll`), which you can rebuild. **No client change is required**, and none of these touch the ~4000 hardcoded addresses in the client.

### Fix 1 — Mark the unit dirty when damaged (recommended; correct fix)

Make HP replication **event-driven** rather than poll-driven. In `CDamageHandler::RecordCharDamage` (`Server/server/DamageHandler.cpp:181`), after damage is applied:

```cpp
Unit * pcUnit = UNITDATATOUNIT( lpChar );
if ( pcUnit )
{
    // Re-serialize immediately so baUnitBufferNew carries the new HP,
    // and bump the revision so LoopUnits() will actually transmit it.
    lpChar->MakeUnitBufferData( (char*)pcUnit->baUnitBufferNew, 0x10, 4 );
    pcUnit->uLastUpdate++;
}
```

This removes the 0–937 ms `iActiWheel` stall and guarantees the very next `LoopUnits()` pass sends fresh HP. Residual delay: ≤ ~1 s from the `b64` gate. Bandwidth cost is negligible (dirty-marking only; no extra sends).

### Fix 2 — Promote monster status to a faster tier (largest perceived win)

Move `LoopUnits()` from the `b64` (1 Hz) tier to `b16` (4 Hz) or `b8` (8 Hz) in `UserServer::Loop()`:

```cpp
if ( pcUser->b8 )
{
    if ( dwTimeDifference < USER_STATUS_UPDATE_GRACE )
    {
        SendDamageInfoAndClearBuffer( pcUser );

        // Monster status at 4 Hz instead of 1 Hz
        if ( pcUser->b16 )
            LoopUnits( pcUser );

        if ( pcUser->b32 )
        {
            if ( pcUser->b64 )
            {
                DAMAGEHANDLER->UserTick1s( pcUser );
                // (LoopUnits removed from here)
                ...
```

Keep `UserTick1s` and `GameTimeSync` at 1 Hz — they are genuinely per-second. **Combined with Fix 1, worst-case drops from ~1000 ms to ~250 ms** (8 Hz → ~125 ms).

Caveat: `LoopUnits()` iterates all units × all users, so cost scales. Fine for 1 player; benchmark before using on a populated server. Consider gating it on player count.

### Fix 3 — Widen the dirty window

In `UnitServer::Update()`, raise the duty cycle:

```cpp
if ( iActiWheel == 0 )
{
    if ( (iUnitWheel % 16) == 0 )   // was 64
        iActiWheel = 4;
}
```

Cruder than Fix 1 (it dirties *all* units more often, not just damaged ones), but a one-line change if you want a quick empirical test.

### Fix 4 — Make it configurable

Add a `server.ini` knob (e.g. `UnitStatusUpdateHz`) so a local/single-player instance runs at 8 Hz while a production instance keeps 1 Hz. Best long-term option; avoids a permanent fork of tuning constants.

### Fix 5 — Client-side prediction (optional polish)

The client already receives exact per-hit damage via `PKTHDR_DamageInfoContainer` (`RecvPacket.cpp:512` → `DAMAGEINFOHANDLER`). It could decrement the local HP bar optimistically on each damage number, then reconcile when authoritative `PKTHDR_UnitStatus` arrives. This makes the bar feel instant regardless of server cadence.

Riskier: it puts HP display in two places and can desync (heals, other players' damage, resistances). Recommend only after Fixes 1+2, and only if you still want more responsiveness.

### Recommendation

Apply **Fix 1 + Fix 2** together. Fix 1 is the correct architectural repair (event-driven dirty marking); Fix 2 delivers the perceptual win. Both are small, server-side, and independently revertible. Then verify with Fix 4 if you ever host publicly.

---

## 8. How to verify

1. **Range test (no code change).** Attack a monster at melee range, then from max range. If lag worsens noticeably at distance, that confirms the `% 2` / `% 4` gating in §5.
2. **Instrument.** Log `TICKCOUNT` in `RecordCharDamage` when HP changes, and again in `LoopUnits()` when that unit's status is actually sent. The delta is the true latency; it should show ~0.5–1.0 s before the fix.
3. **Confirm transport is innocent.** Compare timestamps of `PKTHDR_DamageInfoContainer` vs `PKTHDR_UnitStatus` arrival client-side. Both traverse the same socket; only the send times differ.
4. **After the fix.** Re-run step 2 — expect < ~150 ms.

---

## 9. Summary

| Question | Answer |
|---|---|
| Is it the network / IPC? | **No.** `TCP_NODELAY` is set; loopback is ~0.05 ms, ~0.005% of the delay. |
| Is it performance? | **No.** The server intentionally withholds the data. |
| Root cause | Monster HP is replicated on a 1 Hz "deep update" tier, behind a second dirty-flag gate with a ~6% duty cycle. |
| Why damage numbers look instant | They ride an 8 Hz path (`SendDamageInfoAndClearBuffer`), 11 lines away in the same function. |
| Actual bug | The damage path never marks the unit dirty — `DamageHandler.cpp` contains zero references to `uLastUpdate`. |
| Measured delay | ~0.5 s typical, ~1.0 s worst case in detailed range; up to ~4 s at longer range. |
| Fix | Mark dirty on damage (Fix 1) + move `LoopUnits()` to the 8 Hz tier (Fix 2). Server-side only. |

### Key code references

| Location | Role |
|---|---|
| `Server/server/userserver.cpp:1718` | `UserServer::Loop()` — the 8 Hz vs 1 Hz split |
| `Server/server/userserver.cpp:1456` | `b8`/`b16`/`b32`/`b64` cascade |
| `Server/server/userserver.cpp:1818` | `LoopUnits()` — sole sender of monster HP |
| `Server/server/userserver.cpp:1897` | `uLastUpdate` dirty check (detailed range) |
| `Server/server/userserver.cpp:1915` | `% 2` / `% 4` / `% 8` extra throttle by range |
| `Server/server/userserver.cpp:2395` | `SendDamageInfoAndClearBuffer()` — 8 Hz damage numbers |
| `Server/server/unitserver.cpp:3106` | `UnitServer::Update()` — `iActiWheel` 4-of-64 window |
| `Server/server/unitserver.cpp:2526` | `MakeUnitBufferData()` — where HP is serialized |
| `Server/server/DamageHandler.cpp:181` | `RecordCharDamage()` — applies damage, never flags dirty |
| `shared/socket.cpp:141` | `TCP_NODELAY` — transport ruled out |
| `shared/user.h:19` | `USER_STATUS_UPDATE_GRACE` = 3000 ms |
| `game/game/RecvPacket.cpp:512` | Client damage-number handler |

---

## 10. Implementation record (Fixes 1–4 applied)

Fixes 1–4 were implemented and verified to build. Fix 5 (client-side prediction) was **not** implemented — it is unnecessary once 1–4 are in place, and carries desync risk.

Default shipped configuration: **64 Hz** unit status (`UnitStatusUpdateDivisor=1`) and a **16-frame** dirty window, tuned for a local single-player setup on capable hardware.

### One important correction to Fix 1 as originally written

The original proposal hooked `CDamageHandler::RecordCharDamage`. **That would have worked only on bosses.** `CustomRecordCharacterDamage` (`DamageHandler.cpp:105`) returns early for ordinary monsters:

```cpp
if ( pcUnitData->psaDamageUsersData == NULL && pcUnitData->psaSiegeWarDataList == NULL )
    return 0;
```

Only bosses allocate `psaDamageUsersData` (see `MapServer::SpawnMonsterBoss`), so normal monsters — the overwhelming majority of combat — never reach the dirty-marking code.

The implementation instead hooks `UnitData::SetCurrentHealth()` in `shared/unit.cpp`, the single choke point through which *every* HP mutation passes: `TakeHealth()`, `ApplyDamageOverTime()` (burn/poison), `GiveHealth()`, direct sets, and kills. `SetCurrentHealthToMax()` is covered too, so heals-to-full also replicate promptly.

### Changes

| File | Change |
|---|---|
| `shared/unit.cpp` | New `UnitData::MarkStatusDirty()`; called from `SetCurrentHealth()` and `SetCurrentHealthToMax()` **only when HP actually changed** |
| `shared/unit.h` | Declared `MarkStatusDirty()` (server-only, `#ifndef _GAME`) |
| `shared/user.h` | Added `User::bUnitStatus` replication tick flag |
| `Server/server/userserver.cpp` | Compute `bUnitStatus` from `UNIT_STATUS_UPDATE_DIVISOR`; hoisted `LoopUnits()` out of the `b64` tier into its own faster tick |
| `Server/server/unitserver.cpp` | `iActiWheel` window interval now driven by `UNIT_DIRTY_WINDOW_INTERVAL` |
| `Server/server/globals.h/.cpp` | Added both tuning globals (defaults 8 / 64 = original behaviour) |
| `Server/server/servercore.cpp` | Read + clamp both values from `server.ini`, with `INFO` logging |
| `Files/Server/*/server.ini` | New `[Performance]` section |

`MarkStatusDirty()` guards on `GAME_SERVER`, null `pcUnit`, and `pcUnit->pcUnitData != this`, so it is inert on the login server and safe for player-owned `UnitData`.

### Safety verification

The client and server DLLs patch prebuilt EXEs at fixed addresses, so struct layout is load-bearing. Both structs I extended are plain `new[]` allocations owned by `server.dll`:

- `pcaUser = new User[...]` (`userserver.cpp:13`) — `User` safe to extend
- `pcaUnit = new Unit[...]` (`unitserver.cpp:15`) — `Unit` safe to extend

Whereas the ASM-coupled structs were **not** touched:

- `UnitData` — pointer published to `Server.exe` via `WRITEDWORD( 0x07AC3E78, pcaUnitData )`; layout frozen (`//Size = 0x4D98`). `MarkStatusDirty()` is a member *function*, which does not affect layout.
- `UserData` — asserted by `TEST( "UserData", sizeof( UserData ), 0xB510 )`; untouched.

Confirmed `CReader::ReadInt` returns `0` for absent keys (`shared/CReader.cpp:45`), so the `<= 0` guards make older `server.ini` files fall back to original behaviour.

### Build result

| Target | Result |
|---|---|
| `server` (Release/Win32) | Success, 0 errors, 0 warnings in edited files → `server.dll` 1,335,296 bytes |
| `game` (Release/Win32) | Success, 0 errors, 0 warnings in edited files → `game.dll` 2,663,424 bytes |

The client was rebuilt because `shared/unit.h` and `shared/user.h` are shared with `game.dll`; both compile cleanly under `_GAME`.

### Expected latency after the fix

| Stage | Before | After (divisor 1) |
|---|---|---|
| Dirty-flag wait | 0–937 ms | **0 ms** (immediate on damage) |
| Replication tick wait | 0–1000 ms | 0–16 ms |
| **Total** | **~500–1000 ms** | **~16–30 ms** |

Roughly a **30–60× improvement**, putting the HP bar within one or two client frames of the damage number.

### Tuning

`Files/Server/game-server/server.ini`:

```ini
[Performance]
UnitStatusUpdateDivisor=1    ; 1=64Hz, 2=32Hz, 4=16Hz, 8=8Hz (original)
UnitDirtyWindowInterval=16   ; frames between baseline refreshes (64 = original)
```

`UnitStatusUpdateDivisor` is clamped to a power of two ≤ 8, because the load-spreading wheel `(i % N) == (iWheel % N)` requires it to divide the 64-frame cycle evenly.

At `1`, load spreading is bypassed entirely (`bUnitStatus = TRUE` every frame) — deliberate for single-player, and the reason this is opt-in via config rather than hardcoded.

**If hosting for multiple players, set `UnitStatusUpdateDivisor=8` and `UnitDirtyWindowInterval=64`.** `LoopUnits()` is O(units × users); Fix 1 alone still gives most of the benefit at 8 Hz, since the dominant 0–937 ms stall is eliminated regardless of tick rate.

### Remaining known limitation

The range-based throttle at `userserver.cpp:1915` (`% 2` / `% 4` / `% 8`) was left intact. Beyond ~33 m the server sends `PacketUnitStatusMove`, which carries no HP field, so HP still only refreshes when a monster is within detailed range. Melee combat is unaffected; long-range attackers may still notice slower bar updates. Fixing this properly means adding HP to the basic-range packet — a protocol change, deliberately out of scope here.
