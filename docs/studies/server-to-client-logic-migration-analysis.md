# Migrating Server Logic Into `game.dll` — Feasibility and Strategy

Created on: 2026-07-31

**Scope.** Now that `game.dll` compiles, this study answers: how much of `gameserver` + `loginserver` can be moved into the client, in order to

1. eliminate inter-process communication,
2. minimize packets sent and received,
3. stop gameplay updates being bottlenecked by tick updates,
4. remove update cadence entirely,
5. make `server.dll` as lean as possible.

All figures below were measured against the current working tree, not estimated.

---

## Executive summary

Five findings, in order of importance.

1. **Goals 3, 4 and 5 are achievable and cheap. Goals 1 and 2 are not.** Removing cadence is a bounded, low-risk refactor inside `server.dll`. Removing IPC is blocked by a binary you cannot rebuild. These are separable — and the cheap ones deliver almost all the perceived benefit.

2. **The single biggest win is collapsing `loginserver` + `gameserver` into one process. The code for it already exists and is already wired up.** `SERVERTYPE_Multi` (`Server/server/server.cpp:334`, `:371-374`) sets `g_bLoginServer = g_bGameServer = TRUE`. It is reachable today by setting a negative `ID` in `server.ini`. That deletes **one entire process, one TCP socket, and 61 inter-server packet types** in a config change. See [Part 4](#part-4-the-cheap-win-collapse-the-two-server-processes).

3. **You cannot move gameplay logic into `game.dll`, because a large part of it is not in `server.dll` either.** `server.dll` is a hook layer over a prebuilt closed-source `Server.exe`. It installs **74 function pointers** into `Server.exe`'s dispatch table at `0x08B64338+` (`Server/server/DLL.cpp:872-`) and calls back into EXE machine code at fixed addresses (33 `CALL(0x…)` sites, 500+ hardcoded addresses across 25 files). Monster AI, unit arrays (`UNITSDATA` = `*(UnitData**)0x07AC3E78`), map data (`MAPSDATA` = `*(Map**)0x07AC9CF4`) and the entire spawn/animation core live in the EXE. "Migrating" them means reverse-engineering them. See [Part 2](#part-2-why-you-cannot-move-the-logic).

4. **The cadence is the real cause of your symptoms, and it is entirely fixable in `server.dll` without touching one packet.** There are **five stacked layers** of throttling (`Sleep(15)` → 64 Hz accumulator → per-user 4-tier wheel → per-unit `i % 4` → dirty-window duty cycle). Layers 3–5 exist purely to spread CPU cost across hundreds of players and are pure loss at low population. Two of the five have already been made configurable; the remaining three have not. See [Part 3](#part-3-the-five-layers-of-cadence) and [Part 5](#part-5-how-to-remove-cadence).

5. **`server.dll` cannot be made "lean" by moving code out — only by deleting code that is dead or unwanted.** 91,760 lines of `Server/server` are almost all *content* systems (items 9,389 lines; GM commands 6,183; quests 4,735). Nothing about them is server-ish by nature, but every one of them is entangled with `Server.exe` memory or SQL. The honest lean-ness lever is **feature deletion**, not relocation. See [Part 6](#part-6-what-leanness-actually-means-here).

The one-sentence correction to the framing:

> The bottleneck is not IPC, and never was. It is a **deliberate load-spreading cadence designed for 550 concurrent players**, running on a deployment with one player. Fix the cadence and the IPC becomes irrelevant.

---

## Part 0: The architecture constraint

Established in `docs/studies/game-exe-buildability-and-offline-feasibility.md`, restated here because every conclusion depends on it.

| Binary | Source in repo? | Role |
|---|---|---|
| `Game.exe` | **No** | Prebuilt 2002-era client, imports `WinMain` from `game.dll` |
| `Server.exe` | **No** | Prebuilt server, imports `WinMain` from `server.dll` |
| `game.dll` | Yes | Hook layer, patches `Game.exe` at ~4,000 addresses |
| `server.dll` | Yes | Hook layer, patches `Server.exe` at ~500 addresses |

Both DLLs are *plugins into address spaces you do not control*. This is symmetric and it is the crux: **`game.dll` and `server.dll` cannot be merged, because each is bound to a different EXE's memory layout.** Two different sets of absolute addresses cannot coexist in one process.

Measured coupling on the server side:

```
Server files containing 0x00xxxxxx / 0x07xxxxxx / 0x08xxxxxx addresses:
  itemserver.cpp      212      unitinfo.cpp         36
  DLL.cpp              88      servercommand.cpp    28
  mapserver.cpp        58      unitserver.cpp       26
  servercore.cpp       53      globals.h            26
  bellatraserver.cpp   37      userserver.cpp       17
  … 15 more files
```

The bidirectional binding is explicit. `Server/server/DLL.cpp:872-` installs the callback table:

```cpp
(*(DWORD*)0x08B64338) = (DWORD)&SocketSend;
(*(DWORD*)0x08B6433C) = (DWORD)&OnLoginAccount;
(*(DWORD*)0x08B64340) = (DWORD)&OnLoginSuccess;
// … 74 total
```

`Server.exe` calls *into* `server.dll` through those slots. `server.dll` calls *back into* `Server.exe`:

```cpp
// Server/server/unitserver.cpp:3130
WRITEDWORD( 0x008B8D18, GetTickCount() );
WRITEDWORD( 0x07AC9D68, iActiWheel );
```

Even the master data arrays are EXE-owned (`Server/server/globals.h:172-176`):

```cpp
#define USERSDATA   ( *( UserData** )0x007AAC888 )
#define UNITSDATA   ( *( UnitData** )0x007AC3E78 )
#define MAPSDATA    ( *(Map**)0x07AC9CF4)
#define MAX_UNITS   2048
#define PLAYERS_MAX 1024
```

**Consequence:** the phrase "migrate code from gameserver into game" describes moving a function whose *data* lives at `0x07AC3E78` in a different process. There is no code motion that accomplishes this.

---

## Part 1: What the client already has (and it is more than expected)

This is the genuinely encouraging half of the analysis. The client is not a thin renderer — a substantial amount of gameplay math is already compiled into `game.dll`.

### Shared code = logic already present in both binaries

Per `CMakeLists.txt:20-98`, these `shared/*.cpp` files compile into **both** targets:

| Shared file | Content |
|---|---|
| `unit.cpp` | Unit/monster state, HP get/set, damage application, animation |
| `character.cpp` | Character stat derivation |
| `item.cpp` | Item structures, stat interpretation |
| `map.cpp` | Map/zone data |
| `quest.cpp` | Quest structures |
| `party.cpp` | Party structures |
| `dice.cpp` | RNG |
| `point.cpp` | Geometry / distance |
| `chat.cpp`, `Coin*.cpp` | Chat, coin shop |

Total `shared/` = **29,861 lines**, and the intersection is most of it. The role split is narrow and explicit — only **9 files** use `_GAME` / `_SERVER` conditionals at all, concentrated in `unit.cpp`/`unit.h` (16 sites each).

The pattern in `shared/unit.cpp` is instructive. `GetCurrentHealth()` (`:134`) has two implementations:

```cpp
int UnitData::GetCurrentHealth()
{
#ifdef _GAME
    if ( this->iID == UNITDATA->iID )        // it's me → local truth
        return CHARACTERGAME->GetCurrentHP();
    Unit * pcUnit = UNITDATATOUNIT( this );  // it's someone else → replicated value
    …
#endif
}
```

Whereas the mutators (`SetCurrentHealth`, `TakeHealth`, `GiveHealth`, `ApplyDamageOverTime`, `SetCurrentHealthToMax`) are all `#ifndef _GAME` — **present in the client's source but compiled out.** The authority boundary is a preprocessor flag, not an architectural chasm.

### The client already computes damage inputs

`game/game/EXE.cpp:4415` — the client computes and sends its own attack power:

```cpp
sPacket.AttackPowerMin = CHARACTERGAME->sCharacterDataEx.iBaseAttackPowerMin;
sPacket.AttackPowerMax = CHARACTERGAME->sCharacterDataEx.iBaseAttackPowerMax;
sPacket.sCriticalChance = UNITDATA->sCharacterData.iCritical;
sPacket.iAttackRating   = UNITDATA->sCharacterData.iAttackRating;
```

And the server merely rolls between the client-supplied bounds — `Server/server/DamageHandler.cpp:2894-2902`:

```cpp
sDefAttack[0] = psSkillsDataPacket->AttackPowerMin;   // ← from the client
sDefAttack[1] = psSkillsDataPacket->AttackPowerMax;   // ← from the client
pcUser->iLatestAttackPower = Dice::RandomI( sDefAttack[0], sDefAttack[1] );
TransAttackData.iDamage = pcUser->iLatestAttackPower;
```

The full buff/skill stack that *produces* those numbers is client-side: `game/game/SkillManager.cpp:3812-3956` applies dozens of percentage boosts (Triumph of Valhalla, Advent Migal, Force of Nature, God's Blessing, Summon Muspell, Chasing Hunt…). `game/game/CharacterGame.cpp:410-431` applies Bless Castle crown buffs client-side.

**This is the single most important finding for your goal.** For player→monster damage, the server is already little more than an RNG call and a checksum. The heavy formula work is already in `game.dll`.

### The client already maintains a full unit world

`game/game/globals.h:17` — `MAX_UNITS 1024`, with `UNITGAME->pcaUnitData` iterated directly for rendering, minimap, particles, party HP (`CMiniMapHandler.cpp:421`, `EXE.cpp:503`, `CPartyHandler.cpp:168`). `UNITDATABYID()` gives O(1) lookup. The client is not told *where* things are frame by frame — it interpolates and animates locally.

### What the client genuinely does not have

| Missing | Evidence |
|---|---|
| Any DB access | 0 matches for `SQLConnection|ODBC|sql.h` in `game/game/` |
| Item generation | `ITEMSERVER->CreateItem` / `CreatePerfectItem` exist only server-side (`itemserver.cpp`, 9,389 lines) |
| Drop tables | `lootserver.cpp:409` rolls against `monsterDropTable` loaded from SQL |
| Monster AI | `UnitServer::UpdateUnit` + `MainUnitMonsterData`, and the EXE core it wraps |
| Persistence | `AccountServer::SQLCharacterSave`, warehouse, quest state |
| Spawn tables | `UNITINFODATA->ReadUnitSpawnData` fed from `GameDB` |

---

## Part 2: Why you cannot move the logic

Four blockers, in descending severity. Only the first is fatal.

### Blocker 1 — the logic's data lives in another process's fixed addresses (fatal)

`UnitServer::Update()` (`unitserver.cpp:3106`) is the monster AI driver. It walks `pcaUnit`, which aliases `UNITSDATA` at `0x07AC3E78` — memory owned by `Server.exe`. `UpdateUnit()` calls `MainUnitMonsterData()`, which the source itself admits is not understood (`unitserver.cpp:3220`):

```cpp
//Line 5012 in leaked source code
//This code doesn't work at the moment. The monster doesn't attack players.
BOOL UnitServer::MainUnitMonsterData( UnitData * pcUnitData )
```

Monster attack logic is *already partially broken in the code you have*. Moving it is not porting; it is finishing an unfinished reverse-engineering job.

`UnitServer` itself calls into EXE code by address (`unitserver.h:27-30`):

```cpp
IMPFNC pfnGetTop10DamageUnitData = 0x0055A3D0;
IMPFNC pfnUnitSwapper            = 0x0054FEA0;
IMPFNC pfnUnitDamageSkill        = 0x0054FAB0;
IMPFNC pfnUnitDataByIDMap        = 0x0054CCA0;
```

Same for NPC interaction (`unitserver.h:13-22`): `SEND_SHOP_ITEM_LIST = 0x00551290`, `SEND_OPEN_WAREHOUSE = 0x00551580`, `SEND_QUEST_PROGRESSION = 0x0055AF20`. These are `Server.exe` functions. `game.dll` cannot call them; the addresses mean something entirely different inside `Game.exe`.

### Blocker 2 — persistence is SQL Server

`SQLConnection.h:14-73` enumerates **~40 logical connections** across 10 databases (`GameDB`, `UserDB`, `ServerDB`, `LogDB`, `SkillDB`, `SkillDBNew`, `EventDB`, `ItemDB`, `ClanDB`, `ChatDB`). Content data (monster stats, drop tables, NPC data, quest defs, mixing recipes, item lists) *and* live state (characters, inventory, warehouse) both live there. `docs/analysis/project-analysis.md` records that `ChatDB`, `SkillDB` and `SoD2DB` are **absent** from the runtime pack — a complete dataset is not even on hand.

Moving this into `game.dll` means either linking ODBC into the client, or migrating 10 schemas to SQLite and rewriting every query.

### Blocker 3 — the protocol is the client's state machine

- **380** `PKTHDR_*` identifiers in `shared/packets.h`
- **154** dispatch cases in `game/game/RecvPacket.cpp`
- **148** cases in `Server/server/packetserver.cpp`
- **66** cases in `Server/server/netserver.cpp`

The client is written to *react*. Removing the socket means synthesizing all 154 inbound cases in correct order with correct timing — that is Option C in Part 7, and it degenerates into a rewrite the moment you want combat or loot.

### Blocker 4 — anti-cheat assumes a hostile client

`game/game/AntiCheat.cpp` reports to the *login* server (`SENDPACKETL`) on checksum mismatch (`:271`), debugger presence (`:366`), defense-multiplier tampering (`:381`), HP/MP regen formula tampering (`:393`), speedhack DLLs (`:499`), and `game.dll` module checksum drift (`:407`). If the client becomes authoritative, every one of these is meaningless — and `CHEATLOGID_ModuleSyncError` will fire on your own modified DLL.

---

## Part 3: The five layers of cadence

This is where your actual complaint lives. There is no single tick — there are **five nested ones**, and each multiplies the worst-case latency of the one below.

### Layer 1 — `Sleep(15)` on a dedicated clock thread

`shared/CWindow.cpp:263-298`. A single `UpdaterThread` does nothing but sleep and post:

```cpp
Sleep( dwUpdateTimeInterval );          // :281
QueryPerformanceCounter( &liNewTick );
fTimeStruct.fTime = … ;
SendMessageA( hWnd, WM_UPDATE, … );     // :294  synchronous, blocks until handled
```

`dwUpdateTimeInterval` is **hardcoded to 15** in `Server/server/CServerWindow.cpp:50-51` — note the `CConfigFileReader` is opened and then ignored:

```cpp
CConfigFileReader * pcConfigFileReader = new CConfigFileReader( "server.ini" );
if( pcConfigFileReader->Open() )
{
    //dwUpdateTimeInterval = 100; //10 times per second (10Hz / 10FPS)
    dwUpdateTimeInterval = 15; //~60 times per second (~60FPS)
    pcConfigFileReader->Close();
}
```

`Sleep(15)` on default Windows timer resolution actually yields ~15.6 ms, and it is subject to scheduler granularity. **This is a hard floor: nothing on the server can react faster than ~15 ms.**

### Layer 2 — the 64 Hz fixed-step accumulator

`Server/server/CServerWindow.cpp:144-167`:

```cpp
static double fTick = (1000.0f / ((double)64));   // 15.625 ms
fOffs += fTime;
while( fOffs >= fTick ) { GSERVER->Loop(); fOffs -= fTick; }
GSERVER->Time( fTime, pServer );
```

`Server::Loop()` (`server.cpp:692-713`) is the whole game step:

```cpp
TickCount( GetTickCount() );
USERSERVER->Update();   // per-user wheel flags + status
UNITSERVER->Update();   // monster AI + dirty window
MAPSERVER->Update();    // 67 maps
USERSERVER->Loop();     // all outbound replication
```

Note the mismatch: the clock sleeps 15 ms, the accumulator wants 15.625 ms. They beat against each other, so `Loop()` runs 0, 1, or occasionally 2 times per wake.

Everything is serialized under one mutex — `CServerWindow.cpp:149`: `SERVER_MUTEX->Lock( 3000 )`.

### Layer 3 — the per-user 4-tier wheel

`Server/server/userserver.cpp:1440-1524`. Each user's index `i` is compared to a global `iWheel`:

```cpp
if ( ( i % 8 )  == ( iWheel % 8 ) )                    pcUser->b8  = TRUE;   // 8 Hz
if ( pcUser->b8  && (i % 16) == (iWheel % 16) )        pcUser->b16 = TRUE;   // 4 Hz
if ( pcUser->b16 && (i % 32) == (iWheel % 32) )        pcUser->b32 = TRUE;   // 2 Hz
if ( pcUser->b32 && ( i % 64 ) == ( iWheel % 64 ) )    pcUser->b64 = TRUE;   // 1 Hz
```

Each flag ANDs with its parent, so `b64` is true for a given user once per 64 frames. **`i` is the slot index, so users are deliberately smeared across the wheel** — correct for 550 players, pure latency for one.

Consumers, `userserver.cpp:1712-1837`:

| Tier | Rate | What it gates |
|---|---|---|
| `bUnitStatus` | configurable | `LoopUnits()` — monster/NPC status incl. HP |
| `b8` | 8 Hz | `SendDamageInfoAndClearBuffer()` — damage numbers |
| `b32` | 2 Hz | `LoopUsers()` — other players' status |
| `b64` | 1 Hz | `DAMAGEHANDLER->UserTick1s()`, `PKTHDR_GameTimeSync` |
| `bTenSeconds` | 0.1 Hz | premium timer sync, skill buff broadcast |
| `bOneMin` | 0.017 Hz | premium DB sync, quest expiry, user effect sync |

### Layer 4 — the per-unit `i % 4` serialization slot

`Server/server/unitserver.cpp:3134-3160`. Only a quarter of units are processed per frame:

```cpp
if ( (i % 4) == (iUnitWheel % 4) )
{
    UpdateUnit( pcUnit );                      // AI, movement, aggro, re-serialize
    if ( iActiWheel != 0 ) pcUnit->uLastUpdate++;
}
```

With `MAX_UNITS 2048`, each individual monster's AI runs at **16 Hz**, not 64 Hz.

### Layer 5 — the dirty-window duty cycle

`unitserver.cpp:3121-3127`:

```cpp
if ( iActiWheel == 0 )
{
    if ( (iUnitWheel % (UINT)UNIT_DIRTY_WINDOW_INTERVAL) == 0 )
        iActiWheel = 4;      // window open for 4 frames…
}
else
    iActiWheel--;            // …then closed
```

The baseline dirty flag only advances inside that window.

### Current tuning state

Two of the five layers were already made configurable (see `docs/studies/monster-hp-update-latency-analysis.md`, §10) and are shipped aggressive in `Files/Server/game-server/server.ini:57-71`:

```ini
UnitStatusUpdateDivisor=1     ; 64 Hz — LoopUnits() every frame
UnitDirtyWindowInterval=16    ; ~250 ms baseline window
```

Plus event-driven dirty marking now exists — `shared/unit.cpp:7` `UnitData::MarkStatusDirty()`, called from `SetCurrentHealth()`/`SetCurrentHealthToMax()` only on real change.

**So Layers 4 and 5 are handled. Layers 1, 2 and 3 are not.**

### Residual latency budget

| Layer | Status | Worst case |
|---|---|---|
| 1. `Sleep(15)` clock | **hardcoded** | ~15–16 ms |
| 2. 64 Hz accumulator | **hardcoded** | ~15.6 ms |
| 3. `b8`/`b16`/`b32`/`b64` wheel | **hardcoded** | 125 ms – 1000 ms depending on tier |
| 4. unit `i % 4` | fixed, benign | ~62 ms (AI only) |
| 5. dirty window | configurable + event-driven | ~0 ms for HP |
| Loopback TCP | — | ~0.05 ms |

For monster HP specifically, the earlier fixes already got you to roughly one frame. **For everything still on `b8`/`b32`/`b64` — other players' positions, buff state, damage numbers — you are still paying 125 ms to 1 s.** That is Layer 3, and it is the remaining cadence to kill.

---

## Part 4: The cheap win — collapse the two server processes

**This is the highest value-per-risk action available, and it needs no C++ changes.**

### The role is one config line

`Server/server/server.cpp:314`:

```cpp
iID = pcConfigFileReader->ReadInt( pcConfigFileReader->ReadString( "Server", "Type" ), "ID" );
SERVER_CODE = iID;
```

It reads `Type` as a *section name*, then reads `ID` from that section. The two shipped INIs are byte-identical except line 4 (`Type=LoginServer` vs `Type=GameServer1`).

### `SERVERTYPE_Multi` already exists and already works

`server.cpp:329-343` — if `ID < 0`, the server registers itself as `SERVERTYPE_Multi`:

```cpp
else
{
    iID = 0;
    SERVER_CODE = 0;
    LoadServerInfo( pcConfigFileReader, saServerInfo + 0, SERVERTYPE_Multi, "LoginServer" );
    …
}
```

and `server.cpp:361-375`:

```cpp
case SERVERTYPE_Multi:
    g_bLoginServer = TRUE;
    g_bGameServer  = TRUE;
    break;
```

It is fully plumbed: `server.cpp:378-379` sets the console title to `"Game and Login Server"`, and `CServerWindow.cpp:202-208` renders `"[Multi Server] ONLINE!"`.

Note that `CConfigFileReader::ReadInt` is `atoi()` (`shared/CConfigFileReader.cpp:41-46`), which returns `0` for a missing/garbage key — so a *missing* section yields `0`, not negative. You must set an explicitly negative `ID` to select Multi.

### What collapsing deletes

| Removed | Measure |
|---|---|
| One OS process | `loginserver.exe` |
| One listening socket + 1,000 I/O threads | `iMaxConnections = 500` × 2 threads (`socketserver.cpp:852-866`) |
| The whole inter-server channel | **61** `PKTHDR_Net*` types; 112 uses in `netserver.cpp`, 50 in `socketserver.cpp` |
| The client's second connection | `MAX_CONNECTIONS 2` (`game/game/SocketGame.h:25`), ports 10009 + 10007 |
| Session token round-trip | `PKTHDR_NetPlayerWorldToken` → `NetServer::AddWorldConnectAllowance` (`netserver.cpp:1663`) → `UsePlayerWorldLoginToken` (`:1672`) |
| Per-minute online-count packet | `NetServer::Tick()` (`netserver.cpp:1701-1722`) |
| Cross-process quest/inventory/gold sync | `PKTHDR_NetQuestUpdateDataPart`, `NetPlayerInventory`, `NetPlayerGold`, `NetPlayerGoldDiff`, `NetPlayerItemPut`, `NetPlayerThrow` |

Because both roles then run in one process under one `SERVER_MUTEX`, all of that becomes direct function calls with **zero serialization and zero latency**. This is *exactly* the "prevent interprocess communication" goal — achieved without touching `Game.exe`.

### Residual risk

- `netserver.cpp:1655-1658` branches on `LOGIN_SERVER` to choose `OnReceiveFromGameServer` vs `OnReceiveClient`. In Multi mode both flags are TRUE, so this takes the *login* path for every client packet. **This needs auditing.**
- 94 `LOGIN_SERVER` and 200 `GAME_SERVER` sites exist; most are `if/else` pairs that become ambiguous when both are TRUE. Concentrated in `questserver.cpp` (13), `characterserver.cpp` (12), `packetserver.cpp` (12), `server.cpp` (9), `netserver.cpp` (8), `userserver.cpp` (7).
- `UserServer::Update()` has a hard `if( GAME_SERVER ) … else …` (`userserver.cpp:1454`, `:1526`) — the else branch does login-side inventory hashing. In Multi mode the else branch never runs.
- `UserServer::Loop()` returns early on `LOGIN_SERVER` (`userserver.cpp:1722`) — in Multi mode this would **skip all game replication**. This is a definite bug for Multi and must be fixed first.

**Verdict: Multi mode is a real, coded, ~90%-complete path. Budget a few days to audit the 294 role-gated sites, prioritizing `userserver.cpp:1722` and `netserver.cpp:1655`.**

---

## Part 5: How to remove cadence

Ordered by value/risk. All are `server.dll`-only. **None touch a packet definition, and none touch `game.dll`'s 4,000 addresses.**

### Fix A — make the clock interval configurable and raise it (trivial, high value)

`Server/server/CServerWindow.cpp:50-51` opens `server.ini` and then ignores it. Read the value:

```ini
[Performance]
UpdateIntervalMs=1     ; was hardcoded 15
```

At `1`, combined with `timeBeginPeriod(1)`, the clock thread wakes ~1000×/s and the 64 Hz accumulator in `CServerWindow::Update` fires on schedule instead of beating against a 15 ms floor. Cost: one busy-ish thread. On a single-player box that is free.

Two caveats:
- `winmm` is already linked (`CMakeLists.txt:122`), so `timeBeginPeriod` is available.
- `UpdaterThread` uses `SendMessageA` (`CWindow.cpp:294`), which **blocks until the main thread finishes the frame**. So this cannot run away — it self-limits. Good.

### Fix B — collapse the `b8`/`b16`/`b32`/`b64` wheel at low population (the main remaining win)

`Server/server/userserver.cpp:1472-1524`. The `(i % N) == (iWheel % N)` smearing exists only to spread CPU across many users. Add a divisor exactly as was done for `bUnitStatus`:

```cpp
// [Performance] UserStatusUpdateDivisor — 1 disables load-spreading entirely
if ( USER_STATUS_UPDATE_DIVISOR <= 1 )
{
    pcUser->b8 = pcUser->b16 = pcUser->b32 = TRUE;
    pcUser->b64 = ( ( iWheel % 64 ) == 0 );   // keep 1 Hz semantics for UserTick1s
}
else { /* existing wheel */ }
```

**Important:** `b64` must stay genuinely 1 Hz. `DAMAGEHANDLER->UserTick1s()` and `pcUser->iSecondTick++` (`userserver.cpp:1508-1518`) are *semantically* per-second — regen, DoT, playtime, the `bTenSeconds`/`bOneMin` derivation. Firing them at 64 Hz would multiply regen and DoT by 64. Only `b8`/`b16`/`b32` may be promoted.

Effect: `LoopUsers()` (other players' positions/status) goes from 2 Hz to 64 Hz; damage numbers from 8 Hz to 64 Hz.

### Fix C — make replication event-driven, not poll-driven

This is the architecturally correct fix and the one that genuinely eliminates *cadence* rather than raising its frequency.

`shared/unit.cpp:7` already established the pattern with `MarkStatusDirty()`. Generalize it: on every state change that a client must see, mark dirty and let `Loop()` flush dirty entries only. Then rate becomes "as fast as the frame allows, but only when something changed" — which is what you actually want, and it *reduces* bandwidth at the same time.

Candidate call sites, all of which currently rely on polling:
- `DamageHandler.cpp` — the 9 `CustomRecordCharacterDamage` sites already run through `TakeHealth()`, so HP is covered; buff/debuff application (`iStunTimeLeft`, `bDistortion`, `iIceOverlay`, `bCursed` at `:1666`, `:1733`, `:1895`) is **not**.
- `SendDamageInfoAndClearBuffer` — flush on buffer non-empty rather than on `b8`.
- `LoopUsers()` — flush on position/animation delta rather than on `b32`.

### Fix D — do not chase Layer 4

`(i % 4) == (iUnitWheel % 4)` in `unitserver.cpp:3148` gives 16 Hz monster AI. Raising it to 64 Hz quadruples AI cost across up to 2,048 units for no perceptual gain — animation and movement are interpolated client-side anyway, and `MainUnitMonsterData` is documented as partly non-functional. **Leave it.**

### Fix E — reduce packet *volume*, not just latency

Your goal 2 is minimizing packets. Cadence reduction naively *increases* them. Counter-measures, in order:

1. **Event-driven replication (Fix C) is the answer.** Idle world → near-zero traffic; active combat → high-rate updates. Strictly better than polling on both axes.
2. **Client send rate is already ping-adaptive** — `UnitGame::GetFramesSendCount()` (`game/game/UnitGame.cpp:2471-2483`) returns 16 / 32 / 64 frames by ping. On loopback (ping ≤ 40) it already picks 16, the fastest tier. Consider forcing 16 unconditionally, or adding an 8.
3. **`PKTHDR_PlayDataEx` at ~5 s and `PKTHDR_Ping` at ~3 s** (`packetserver.cpp:1696-1698`) are pure heartbeat. On loopback they are waste; on a Multi server the `PKTHDR_NetPlayDataEx` relay disappears entirely.
4. **`USER_STATUS_UPDATE_GRACE` = 3000 ms** (`shared/user.h:19`) suppresses *all* updates if the server hasn't heard from you in 3 s (`userserver.cpp:1735`, `:1747`). Harmless locally, but it means an idle client sees a frozen world — worth knowing when testing.

### What to expect

| | Now | After A + B + C |
|---|---|---|
| Server clock floor | ~15.6 ms | ~1 ms |
| Monster HP | ~1 frame (already fixed) | ~1 frame |
| Other players' status | up to 500 ms | ~1 frame, on change |
| Damage numbers | up to 125 ms | ~1 frame, on change |
| Buffs/debuffs | up to 500 ms | on change |
| Per-second logic | 1 Hz (correct) | 1 Hz (unchanged, deliberately) |
| Idle bandwidth | constant polling | near zero |

---

## Part 6: What "leanness" actually means here

`Server/server` is **91,760 lines** across 158 files. Distribution:

| File | Lines | Nature |
|---|---|---|
| `itemserver.cpp` | 9,389 | content + item generation |
| `servercommand.cpp` | 6,183 | GM commands |
| `questserver.cpp` | 4,735 | content |
| `mapserver.cpp` | 4,138 | world/spawn |
| `DamageHandler.cpp` | 3,923 | combat resolution |
| `unitserver.cpp` | 3,602 | monster AI |
| `characterserver.cpp` | 2,866 | EXP/leveling |
| `bellatraserver.cpp` | 2,843 | one event mode |
| `userserver.cpp` | 2,706 | session + replication |
| `HNSSkill.cpp` + 9 class files | ~9,300 | skills |
| `blesscastleserver.cpp` + handler | ~2,000 | siege event |
| `FuryArenaHandler.cpp` | 1,394 | one event mode |
| `CoinShopHandler.cpp` | 1,313 | cash shop |
| `logserver.cpp` | 1,356 | telemetry |

**Almost none of this is "server infrastructure."** It is game content. And that is precisely why it cannot move: content logic here reads `Server.exe` memory (`itemserver.cpp` alone has 212 hardcoded addresses) and writes SQL.

So leanness comes from **deletion**, not relocation. Realistic candidates for a single-player / small deployment:

| Delete | Lines | Justification |
|---|---|---|
| `bellatraserver.cpp` + handler | ~2,900 | competitive event mode |
| `blesscastleserver.cpp` + `BlessCastleHandler.cpp` | ~2,000 | clan siege |
| `FuryArenaHandler.cpp`, `QuestArenaHandler.cpp` | ~2,000 | PvP events |
| `CoinShopHandler.cpp` + `Coin*.cpp` | ~1,800 | cash shop |
| Seasonal handlers (`Christmas`, `Easter`, `Halloween`, `EventGirl`, `Age`) | ~2,500 | seasonal events |
| `BotServerHandler`, `BotShopServerHandler`, `CBotServerAIHandler` | ~1,500 | bots |
| `pvpserver.cpp`, `RankingListHandler.cpp` | ~1,000 | ladder |
| `logserver.cpp` packet/DB telemetry | ~800 | operational telemetry |
| `cheatserver.cpp` + anti-cheat reporting | ~500 | meaningless when you own the client |
| `CServerManager.cpp` | 23 | **dead stub** — `Init()` returns TRUE, `Shutdown()` empty |

Realistic ceiling: **~15,000 lines, ≈16%**. Each deletion needs its `PKTHDR_*` cases pruned from `packetserver.cpp` and its `server.ini` keys removed. This is safe, incremental work — but it is a diet, not a transplant.

Also worth removing on the *client* side: `AntiCheat.cpp` (28.6 KB), `CAntiDebuggerHandler.cpp`, and the `LdrLoadDll` anti-injection hook in `Main.cpp:398`+. These actively obstruct your own development (`CHEATLOGID_ModuleSyncError` fires on your own rebuilt `game.dll`).

---

## Part 7: Options, honestly assessed

| Option | Effort | IPC removed | Cadence removed | Verdict |
|---|---|---|---|---|
| **A. `SERVERTYPE_Multi`** — one server process, delete the login/game split | Days | **Yes, all inter-server** | No | **Do this first.** Coded already; kills 61 packet types and 1,000 threads. |
| **B. Cadence removal (Fixes A+B+C)** | Days–weeks | No | **Yes** | **Do this second.** Directly fixes the felt problem; contained in `server.dll`. |
| **C. Feature deletion** | Weeks, incremental | No | No | **Do this third.** Real leanness, safe, revertible. |
| **D. Merge `server.dll` into `game.dll`** | — | Yes | Yes | **Impossible.** Two DLLs bound to two different EXEs' absolute addresses; cannot share one address space. |
| **E. Move gameplay logic client-side, keep sockets** | Months–years | Partly | Yes | Requires reverse-engineering `Server.exe`, plus SQL→embedded migration, plus resolving `MainUnitMonsterData`. Becomes a rewrite. |
| **F. In-process packet emulator** — replace the socket with a shim | Many months | Yes | Yes | Must answer 154 inbound cases correctly. Fine for "log in and walk around"; collapses into E for real gameplay. |
| **G. Full reimplementation** | Years | Yes | Yes | Only true single-process path. No longer this codebase. |

### Recommendation

**A → B → C, in that order.** Together they deliver:

- **Goal 1 (no IPC):** ✅ achieved for inter-server via A. Client↔server loopback remains, at ~0.05 ms — 0.005% of your current latency. Not worth attacking.
- **Goal 2 (fewer packets):** ✅ via A (61 packet types gone) + Fix C (event-driven ⇒ idle traffic → ~0).
- **Goal 3 (no tick bottleneck):** ✅ via Fixes A + B.
- **Goal 4 (no cadence):** ✅ via Fix C, except the ~6 genuinely per-second systems that *must* stay 1 Hz (regen, DoT, playtime).
- **Goal 5 (lean `server.dll`):** ⚠️ partially — ~16% by deletion. Relocation is not available.

### What to do first, concretely

1. Fix `UserServer::Loop()`'s `if ( LOGIN_SERVER ) return;` (`userserver.cpp:1722`) so it does not skip game replication in Multi mode. **Blocker for Option A.**
2. Audit `netserver.cpp:1655-1658` (`OnReceiveFromGameServer` vs `OnReceiveClient`) for Multi.
3. Set a negative `ID` in one `server.ini`, boot Multi, confirm `"[Multi Server] ONLINE!"`, then delete the second server directory.
4. Read `UpdateIntervalMs` in `CServerWindow.cpp:50` and add `timeBeginPeriod(1)`.
5. Add `UserStatusUpdateDivisor` alongside the existing `UnitStatusUpdateDivisor`, keeping `b64` at true 1 Hz.
6. Extend `MarkStatusDirty()` to buffs, positions and the damage buffer.

Each step is independently testable and independently revertible.

---

## Verification appendix

Measurements taken against the current working tree:

| Metric | Value | Source |
|---|---|---|
| `Server/server` LOC | 91,760 | `.cpp` + `.h` line count |
| `game/game` LOC | 139,099 | same |
| `shared` LOC | 29,861 | same |
| `PKTHDR_*` identifiers | 380 | `shared/packets.h` |
| `PKTHDR_Net*` (inter-server) | 61 | `shared/packets.h` |
| Client dispatch cases | 154 | `game/game/RecvPacket.cpp` |
| Server dispatch cases | 148 | `Server/server/packetserver.cpp` |
| Inter-server dispatch cases | 66 | `Server/server/netserver.cpp` |
| `LOGIN_SERVER` sites | 94 | `Server/server/*.cpp` |
| `GAME_SERVER` sites | 200 | `Server/server/*.cpp` |
| EXE callback pointers installed | 74 | `Server/server/DLL.cpp` |
| `CALL(0x…)` into `Server.exe` | 33 | `Server/server/*.cpp` |
| Server files w/ EXE addresses | 25 | `Server/server/*.cpp,*.h` |
| Shared files w/ `_GAME`/`_SERVER` splits | 9 | `shared/*.cpp,*.h` |
| SQL logical connections | ~40 across 10 DBs | `Server/server/SQLConnection.h:14-73` |
| Socket threads | 500 × 2 | `Server/server/socketserver.cpp:852-866` |
| SQL/ODBC references in client | **0** | `game/game/*` |

### Key code references

| Location | Role |
|---|---|
| `Server/server/CServerWindow.cpp:50` | `dwUpdateTimeInterval = 15` — hardcoded clock (Layer 1) |
| `shared/CWindow.cpp:263-298` | `UpdaterThread` — `Sleep` + `SendMessageA(WM_UPDATE)` |
| `Server/server/CServerWindow.cpp:144-167` | 64 Hz accumulator + `SERVER_MUTEX` (Layer 2) |
| `Server/server/server.cpp:692-713` | `Server::Loop()` — the game step |
| `Server/server/server.cpp:715-768` | `Server::Time()` — 500 ms / 1 s / 10 s / 1 min / 1 h ticks |
| `Server/server/userserver.cpp:1440-1524` | `b8`/`b16`/`b32`/`b64` wheel (Layer 3) |
| `Server/server/userserver.cpp:1712-1837` | `UserServer::Loop()` — all outbound replication |
| `Server/server/userserver.cpp:1722` | `if ( LOGIN_SERVER ) return;` — **Multi-mode blocker** |
| `Server/server/unitserver.cpp:3134-3160` | unit `i % 4` slot (Layer 4) |
| `Server/server/unitserver.cpp:3121-3127` | `iActiWheel` dirty window (Layer 5) |
| `shared/unit.cpp:7` | `MarkStatusDirty()` — the event-driven pattern to generalize |
| `Server/server/server.cpp:314` | `Type` → `ID` role selection |
| `Server/server/server.cpp:329-343`, `:361-375` | `SERVERTYPE_Multi` — the collapse path |
| `Server/server/netserver.cpp:1655-1658` | login vs client receive branch — audit for Multi |
| `Server/server/netserver.cpp:1701-1722` | `NetServer::Tick()` — per-minute inter-server packet |
| `Server/server/DLL.cpp:872-` | 74 EXE callback pointers |
| `Server/server/globals.h:172-176` | `USERSDATA` / `UNITSDATA` / `MAPSDATA` in EXE memory |
| `Server/server/unitserver.h:13-30` | EXE function addresses used by `UnitServer` |
| `Server/server/DamageHandler.cpp:2894-2902` | server rolls between **client-supplied** damage bounds |
| `game/game/EXE.cpp:4415` | client computes and sends its own attack power |
| `game/game/SkillManager.cpp:3812-3956` | client-side buff/skill damage stack |
| `game/game/UnitGame.cpp:2471-2483` | `GetFramesSendCount()` — ping-adaptive client send rate |
| `game/game/CGameWindow.cpp:476-499` | client 60 Hz accumulator |
| `game/game/SocketGame.h:25` | `MAX_CONNECTIONS 2` — the two sockets Option A removes |
| `Server/server/packetserver.cpp:1685-1713` | the periodic-packet inventory, with intervals documented |
| `shared/user.h:19` | `USER_STATUS_UPDATE_GRACE` = 3000 ms |
| `Files/Server/game-server/server.ini:57-71` | existing `[Performance]` knobs |

### Related documentation

- `docs/studies/game-exe-buildability-and-offline-feasibility.md` — why `Game.exe`/`Server.exe` cannot be rebuilt
- `docs/studies/monster-hp-update-latency-analysis.md` — Layers 4/5, already fixed
- `docs/analysis/project-analysis.md` — runtime pack gaps, missing databases
- `docs/guides/server-start-guide.md` — current two-process startup that Option A eliminates
