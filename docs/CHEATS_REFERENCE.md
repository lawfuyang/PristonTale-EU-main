# Priston Tale — Complete Cheats & Modifications Reference

## 0. Common GetItem Codes
/getitem BI108

## 1. Chat Commands (type `/activategm` first)

### EXP / Level

| Command | Effect |
|---------|--------|
| `/expevent <0-1000>` | Global EXP bonus % (100 = +100%) |
| `/!giveexp <amount>` | Add raw EXP to yourself |
| `/!levelup <level>` | Set level instantly |

### Gold / Items

| Command | Effect |
|---------|--------|
| `/GetGold <amount>` | Add gold |
| `/GetCoins <amount>` | Add Coin Shop credits |
| `/getitem <code> [class] [spec] [age] [rarity] [perfect]` | Spawn item (e.g. `/getitem wa131`) |
| `/getitemperf <code> <spec>` | Spawn perfect item |
| `/gethpg <count>` | Grand HP Potions |

### Teleport

| Command | Effect |
|---------|--------|
| `/wrap <mapId> <x> <z>` | Teleport to map (e.g. `/wrap 3 0 0` = Ricarten) |
| `/near <char>` | Teleport to player |
| `/call <char>` | Pull player to you |
| `/WarpAll` | Warp everyone to Ricarten |
| `/WarpGarden` | Warp everyone to Garden of Freedom |
| `/WarpEvent` | Warp everyone to Mystery Forest |

### Drop / Monster Mods

| Command | Effect |
|---------|--------|
| `/extradrop <count>` | Extra drops per monster |

### Quest

| Command | Effect |
|---------|--------|
| `/activequests` | List your active quest IDs (for use with `/finishquest`) |
| `/finishquest <id>` | Force-complete a quest by ID |
| `/getquestitem <class> <rank>` | Get tier-3 quest weapon (class: fs/as/ms, rank: 1-5) |
| `/ranktier <0-3>` | Force-set class tier (0=T1, 1=T2, 2=T3, 3=T4). Relog to apply. |

### Premium Buffs / All Players

| Command | Effect |
|---------|--------|
| `/BONUSALL` | Apply premium buffs to ALL online players (HP/MP/SP regen, damage, absorb, speed, EXP, drop) |

### Server Control

| Command | Effect |
|---------|--------|
| `/serverfps <fps>` | Change server tick rate (15-1000) |
| `/shiftgametime <0-24>` | Shift day/night cycle |
| `/force_night_mode <1/0>` | Force night |
| `/force_day_mode <1/0>` | Force day |
| `/spawnbosses` | Force all bosses to spawn now |

### Client-Side GM Commands (need GM mode enabled)

| Command | Effect |
|---------|--------|
| `/SetFSP <n>` | Set Free Skill Points |
| `/SetNoDelay` | Remove all skill cooldowns/delays |
| `/ST <0-24>` / `/SetTime <0-24>` | Set game time of day |

---

## 2. `server.ini` Values (in both `login-server` and `game-server`)

### `[Event]` Section

| Key | Values | Effect |
|-----|--------|--------|
| `RateExp=1` | Multiplier | Base EXP rate (1 = normal) |
| `EventExp=0` | 0-1000 | Additional EXP bonus % |
| `AgingFree=Off` | On/Off | Free aging at startup |
| `Halloween=Off` | On/Off | Halloween event at startup |
| `Xmas=Off` | On/Off | Christmas event at startup |
| `Easter=Off` | On/Off | Easter event at startup |
| `ValentineDay=Off` | On/Off | Valentine event at startup |
| `SiegeWar=On` | On/Off | Siege War enabled |
| `Bellatra=On` | On/Off | Bellatra (SoD) enabled |
| `BellatraTax=10` | Number | Bellatra entrance tax % |
| `BellatraDivScore=15` | Number | Bellatra score divisor |
| `TheGrandFury=Off` | On/Off | Grand Fury event |
| `WantedMoriph=Off` | On/Off | Wanted Moriph event |
| `WantedWolf=Off` | On/Off | Wanted Wolf event |
| `MonsterDamageReduce=Off` | On/Off | Reduce monster damage |
| `SkillMPCostPercent=100` | 0-100 | Skill mana cost % (50 = half, 0 = free) |
| `SkillSPCostPercent=100` | 0-100 | Skill stamina cost % (50 = half, 0 = free) |
| `AlwaysAgingSuccess=Off` | On/Off | Never fail or break on aging (always +2) |
| `MoveSpeedPercent=100` | 1-1000 | Player movement speed % (200 = double, 500 = 5x) |
| `LootMode=0` | 0 or 1 | Cheat loot: 1 = no gold, always perfect, class match, always spec |

### `[Server]` Section

| Key | Values | Effect |
|-----|--------|--------|
| `Version=1017` | Number | Server version (must match client) |
| `MaxUsers=550` | Number | Maximum concurrent players |
| `GameServers=1` | Number | Number of game server instances |

### `[Database]` Section

| Key | Values | Effect |
|-----|--------|--------|
| `Driver={ODBC Driver 18 for SQL Server}` | ODBC driver name | Database driver |
| `Host=127.0.0.1,1433` | IP,port | SQL Server address |
| `User=sa` | String | SQL username |
| `Password=632514Go` | String | SQL password |

---

### EXP Rates

**File:** `Server/server/server.cpp` or `server.ini`
- `RateExp` in `server.ini` controls base rate
- `EventExp` in `server.ini` controls event bonus
- Runtime: `/expevent` command sets `*(int*)0x0084601C`

### Aging

**File:** `Server/server/AgeHandler.cpp`
- `AgingFree` / `AgingNoBreak` globals checked during aging
- Set via `server.ini` `[Event]` section or `/event_agingfree` command

### Monster Stats (Live Edit)

**File:** `Server/server/servercommand.cpp` (Admin command handler)
- `/sql_HP <id> <value>` — changes HP in DB
- `/sql_EXP <id> <value>` — changes EXP in DB
- All `/sql_*` commands edit `GameDB.dbo.MonsterList`

### Drop Rates

**File:** `Server/server/lootserver.cpp`
- `EVENT_EXTRADROPS` global controls extra drops
- Set via `/extradrop` command or modify `EVENT_EXTRADROPS` default

### Mana / HP Costs

**File:** `Server/server/DamageHandler.cpp`
- Skill costs are calculated in `RecvBuffSkill()`, `RecvSkillSingleTarget()`, etc.
- Skill data comes from `SkillDBNew.dbo.SkillData`

### Skill Mana / Stamina Cost Multiplier (NEW)

**Files modified:**
- `Server/server/globals.h` — extern `SKILL_MP_COST_PERCENT`, `SKILL_SP_COST_PERCENT`
- `Server/server/globals.cpp` — defaults (100 = normal)
- `Server/server/servercore.cpp` — reads `SkillMPCostPercent` and `SkillSPCostPercent` from `server.ini [Event]`
- `Server/server/HNSSkill.cpp` — applies multiplier when loading `MPCost` values from DB

**Usage:** Set in `server.ini` both files:
```ini
SkillMPCostPercent=50   # 50% mana cost
SkillSPCostPercent=0    # free stamina
```

`100` = normal, `50` = half, `0` = free. Requires server restart.

### Character Gold / Level

**File:** `Server/server/characterserver.cpp`
- `GiveEXP()` and `GiveGOLD()` methods
- Level-up formula in `GetExpFromLevel()`

### Server Tick Rate (FPS)

**File:** `Server/server/servercommand.cpp`
- `/serverfps` command modifies `*(int*)0x006E46F4`
- Default is ~60; lower = slower server, higher = faster ticks

### Item Creation Perfection

**File:** `Server/server/servercommand.cpp`
- `/getitem` command allows specifying spec, age, rarity, and "perfect" flag
- Setting `*(UINT*)0x8B70264 = 1` forces perfect item creation

### Cheat Detection (Already Disabled)

**Files modified:**
- `Server/server/logserver.cpp` — `OnLogCheat()` returns TRUE immediately
- `Server/server/cheatserver.cpp` — `CheckStatePoint()` returns immediately
- `Server/server/packetserver.cpp` — CheatEngine detection commented out
