# Game Client Buildability and Offline (Serverless) Feasibility Study

Created on: 2026-07-31

Scope of this study:

- whether `game.exe` can be compiled from this repository
- whether a `game.dll` can be compiled to intercept function hooks
- what is missing, and what must change to make it compilable
- whether a single `game.exe` running everything locally with no server is achievable

All conclusions below were produced by actually building the project on this machine, not by reading the code alone.

---

## Executive summary

Three findings, in order of importance.

1. **`game.exe` cannot be compiled from this repository, and it never could be.** There is no `game.exe` source in this repo. `Game.exe` is a closed-source, prebuilt 2002-era Korean binary. This repository builds `game.dll`, which is a mod layer injected into that binary.

2. **`game.dll` compiles successfully right now, with zero source changes.** The reported "cannot compile" is a build-invocation problem, not a code problem. It was reproduced and fixed. See [Reproduction matrix](#reproduction-matrix).

3. **A fully serverless single-`game.exe` is not achievable as a patch, port, or refactor.** It is achievable only as a *rewrite* of the server as an in-process simulation. The blocker is not the network layer; it is that ~108,000 lines of authoritative game logic live in the server and the client contains no replacement for it. See [Part 3](#part-3-serverless-single-exe-feasibility).

The single most important correction to the project's mental model:

> This repository is **not** a game. It is a **hook DLL for a game you do not have the source to**.

---

## Part 0: The architecture you are actually working with

This has to be established first, because questions 1 and 3 are both unanswerable without it.

### The three executables

| Binary | Source in this repo? | What it actually is |
|---|---|---|
| `Game.exe` | **No** | Prebuilt closed-source client, 7,655,424 bytes, dated 2022-02-09 |
| `Server.exe` | **No** | Prebuilt closed-source server, 6,369,280 bytes, dated 2022-02-09 |
| `loginserver.exe` | **No** | Same `Server.exe` binary, different `server.ini` role |

What this repo *does* build:

| Target | Source | Output |
|---|---|---|
| `game.dll` | `game/game/` + `shared/` | 2,715,648 bytes (built during this study) |
| `server.dll` | `Server/server/` + `shared/` | 1,333,760 bytes |

### The injection mechanism

Both DLLs use the identical trick. From `game/game/export.def`:

```
LIBRARY game
EXPORTS
	WinMain
```

And `Server/server/export.def`:

```
LIBRARY Server
EXPORTS
	WinMain
```

The DLL exports `WinMain`. The prebuilt EXE **statically imports `WinMain` from the DLL** and calls it as its entry point. Verified against the shipped runtime:

```
Game.exe refs game.dll: True
Game.exe refs WinMain:  True
```

And on the freshly built DLL:

```
ordinal hint RVA      name
      1    0 000D02C0 WinMain = _WinMain@16
```

So control flow at launch is:

```
Game.exe (prebuilt, closed source)
   └─ imports WinMain from game.dll
        └─ game.dll!WinMain  ← game/game/Main.cpp:394
             ├─ HookExceptionHandler()
             ├─ GameCore::Hooks()          ← patches Game.exe machine code in memory
             ├─ ProtectProcess()
             ├─ HookAntiInjection()
             └─ new CApplication(new CGameWindow()) → Run()
```

`game.dll` is the entry point of the process, but it does not own the process. It runs *inside* `Game.exe`'s address space and rewrites it.

### The consequence: ~4,000 hardcoded addresses

`GameCore::Hooks()` in `game/game/GameCore.cpp:226` begins by making `Game.exe`'s code section writable and patching it:

```cpp
DWORD dwOld = 0;
VirtualProtect( (void*)0x00401000, 0x3AC000, PAGE_EXECUTE_READWRITE, &dwOld );
```

It then patches raw instruction bytes, e.g. raising render limits by overwriting immediates:

```cpp
WRITEDWORD( 0x0047E571, 0x40 * iRenderVertexTotal );
WRITEDWORD( 0x0047E582, (0x40 * iRenderVertexTotal) / 4 );
```

And calls functions that exist only inside `Game.exe`:

```cpp
CALL( 0x00484B40 );
CALL( 0x00448D30 );
```

Measured density of this coupling in `game/game/`:

- **3,997** hardcoded absolute addresses of the form `0x00xxxxxx`
- **26 files** containing direct `CALL(0x00…)` into `Game.exe`

Representative examples of how deep it goes — from `game/game/GameCore.h`:

```cpp
#define WINDOW_ISOPEN_INVENTORY   (*(BOOL*)0x035EBB20)
#define WINDOW_ISOPEN_WAREHOUSE   (*(BOOL*)0x036932FC)
#define GAME_LOADINGTIME          (*(DWORD*)0x03A9767C)

ItemData * pItemsInventory = (ItemData*)0x035EBB24;
CManageWindow * pcManageWindow = (CManageWindow*)(0x03904600);

static BOOL IsSaved()   { return (CALL( 0x00620CA0 ) == 1); }
static BOOL IsWalking() { return (*(BOOL*)0x035E11D0); }
```

Even the login textboxes live in the EXE, per `game/game/DLL.h`:

```cpp
enum class ELoginScreenInputBox
{
	USERNAME = (DWORD)0x039033E8,
	PASSWORD = (DWORD)0x039032E8
};
```

**Implication:** the built `game.dll` is only valid against *one exact build* of `Game.exe`. Every address is a hard dependency on that specific binary layout. This is the fact that makes Part 3 hard, and it is why "compile game.exe" is not a meaningful goal.

The server side is architecturally identical — `server.dll` also patches its prebuilt `Server.exe` (42 files contain `0x004xxxxx`-class addresses, e.g. `Server/server/unitserver.cpp:3125`):

```cpp
WRITEDWORD( 0x008B8D18, GetTickCount() );
WRITEDWORD( 0x07AC9D68, iActiWheel );
```

---

## Part 1: Can you compile it?

### Answer

| Question | Answer |
|---|---|
| Can you compile `game.exe`? | **No — and this is not a defect.** No source exists. Nothing is "missing" that could be added. |
| Can you compile a `game.dll` to intercept function hooks? | **Yes. Verified. It builds cleanly today with no source edits.** |

### Proof: the build was performed

Command that succeeded:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
  "game\game\game.vcxproj" `
  /p:Configuration=Release /p:Platform=Win32 `
  "/p:SolutionDir=c:\Workspace WIth Spaces\PristonTale-EU-main\\"
```

Result:

```
ExitCode=0
game.vcxproj -> c:\Workspace WIth Spaces\bin\Game\game.dll
```

Artifacts produced:

| File | Size |
|---|---|
| `game.dll` | 2,715,648 bytes |
| `game.lib` | 1,674 bytes |
| `game.exp` | 937 bytes |
| `game.pdb` | 22,777,856 bytes |

For reference, the shipped runtime `Files/Game/game.dll` is 2,628,608 bytes. The freshly built DLL is the same order of magnitude, and exports `WinMain` correctly. **The client source tree is healthy.**

### Reproduction matrix

This explains precisely why it was reported as unbuildable. The source is fine; the *invocation* determines success.

| # | How it is invoked | Result | Error |
|---|---|---|---|
| 1 | `MSBuild PristonTale.sln /t:game /p:Platform=Win32` | **FAIL** | `MSB4126: The specified solution configuration "Release\|Win32" is invalid` |
| 2 | `MSBuild game\game.sln /p:Configuration=Release /p:Platform=Win32` | **FAIL** | `LNK1104: cannot open file 'd3dx9.lib'` |
| 3 | `MSBuild game\game\game.vcxproj` (no `SolutionDir`) | **FAIL** | `C1083: Cannot open include file: 'd3dx9.h'` |
| 4 | `MSBuild game\game\game.vcxproj` **with explicit `SolutionDir`** | **SUCCESS** | — |
| 5 | Same as #4 but `Configuration=Debug` | **FAIL** | `C1083: Cannot open include file: 'd3dx9.h'` |

### Root causes

There are four distinct defects. None are in the C++ source.

#### Cause A — the root solution has no `Win32` platform (breaks #1)

`PristonTale.sln` declares only `x64` and `x86` as solution platforms:

```
Debug|x64 = Debug|x64
Debug|x86 = Debug|x86
GameServer|x64 = GameServer|x64
GameServer|x86 = GameServer|x86
Release|x64 = Release|x64
Release|x86 = Release|x86
```

It then maps `x86` onto the project's real `Win32` platform:

```
{2E19552E-...}.Release|x86.ActiveCfg = Release|Win32
```

So the correct solution-level invocation is `/p:Platform=x86`, **not** `Win32`. Passing `Win32` — the intuitive choice, and the one the project itself uses internally — fails. This alone would make someone conclude the client is unbuildable.

#### Cause B — DirectX SDK paths depend on `$(SolutionDir)` (breaks #3, and #2 partially)

`game/game/game.vcxproj`, Release configuration:

```xml
<IncludePath>$(SolutionDir)Shared;$(SolutionDir)deps\dks\Include;$(IncludePath)</IncludePath>
<LibraryPath>$(SolutionDir)deps\dks\Lib\x86;$(LibraryPath)</LibraryPath>
```

The vendored DirectX SDK is committed at `deps/dks/` — `d3dx9.h` and `d3dx9.lib` are both present, so nothing is actually absent from disk. But:

- when building the `.vcxproj` **directly**, MSBuild defaults `$(SolutionDir)` to the *project* directory (`game/game/`), so the path resolves to the non-existent `game/game/deps/dks/Include` → `C1083`
- when building via `game/game.sln`, `$(SolutionDir)` becomes `game/`, so it resolves to the non-existent `game/deps/dks/` → headers are found via the *other* include path but `d3dx9.lib` is not → `LNK1104`

This is why the nested `game/game.sln` is broken: it is one directory level too deep for the `deps/` layout. Only the root `PristonTale.sln` sets `$(SolutionDir)` correctly.

Also note the output path escapes the repo entirely:

```xml
<OutDir>$(SolutionDir)..\bin\Game\</OutDir>
```

With the root solution this writes to `c:\Workspace WIth Spaces\bin\Game\` — *outside* the workspace. That is where the DLL built during this study landed.

#### Cause C — the Debug configuration requires an installed DirectX SDK (breaks #5)

Debug does not use the vendored SDK at all. It relies on the legacy `DXSDK_DIR` environment variable:

```xml
<IncludePath>../../shared/;$(DXSDK_DIR)Include;$(IncludePath)</IncludePath>
<LibraryPath>$(DXSDK_DIR)LIB\x86;$(LibraryPath)</LibraryPath>
```

On this machine `DXSDK_DIR` is **not set**, so those collapse to `Include` / `LIB\x86` and Debug cannot build. Debug additionally hardcodes a foreign post-build path:

```xml
<Command>xcopy "$(OutDir)$(TargetName)$(TargetExt)" "C:\Pristontale EU\" /d /y</Command>
```

#### Cause D — a stale absolute path in a property sheet

`game/game/PropertySheet.props` still points at a developer's old machine:

```xml
<SHARED_DIR>E:\Pristontale\PTEUREVAMPED\Source Viet\shared</SHARED_DIR>
```

Harmless today (the `SHARED_DIR` macro is unused by the current include paths) but it is misleading and should be corrected.

### What is missing — the definitive list

**Missing source files: none.** Verified programmatically against `game.vcxproj`:

- files listed in the project but absent from disk: **0** (`.cpp` and `.h` alike)
- `resource.rc`, `lua.lib`, `discord-rpc.lib`, `discord-rpc.h`, `cpp.hint`, `export.def`: all present
- `deps/dks/Include/d3dx9.h` and `deps/dks/Lib/x86/d3dx9.lib`: both present

**Not missing, but worth knowing — 31 orphaned `.cpp` files** exist on disk and are *not* in the project, so they are silently never compiled:

```
CPremiumHandler.cpp   DX.cpp              DXAudioEngine.cpp   DXColor.cpp
DXFontFactory.cpp     DXFunctions.cpp     DXLens.cpp          DXMaterial.cpp
DXMesh.cpp            DXMeshPart.cpp      DXModel.cpp         DXModelFactory.cpp
DXMusic.cpp           DXSound.cpp         DXSoundCaptureDevice.cpp
DXSoundDevice.cpp     DXSoundFactory.cpp  DXSoundManager.cpp  DXSoundStream.cpp
DXTerrain.cpp         DXTerrainFactory.cpp FullZoomMap.cpp    PremiumModel.cpp
PremiumView.cpp       TestUI.cpp          UIController.cpp    UIFont.cpp
UILine.cpp            UIMessageBox.cpp    UIScroller.cpp      writeClientField.cpp
```

These are dead/experimental code, not build breakage. Do not add them blindly — several are alternative implementations that would cause duplicate-symbol errors.

**Missing tooling (environmental, not repo):**

- `DXSDK_DIR` unset → Debug config unbuildable
- MSVC v143 toolset — satisfied; VS 18 provides `v150/v160/v170/v180` and MSVC `14.29.30133 / 14.44.35207 / 14.51.36231`

### Recommended changes

Ordered by value. The first one alone unblocks you.

**1. Document/script the one correct invocation (zero risk, immediate).**

```powershell
$vs = "C:\Program Files\Microsoft Visual Studio\18\Community"
$repo = "c:\Workspace WIth Spaces\PristonTale-EU-main"
& "$vs\MSBuild\Current\Bin\MSBuild.exe" "$repo\game\game\game.vcxproj" `
  /p:Configuration=Release /p:Platform=Win32 "/p:SolutionDir=$repo\\" /m
```

Worth adding as `scripts/build-pt-client.ps1` to match the existing script conventions.

**2. Make the project independent of `$(SolutionDir)` (small, high value).**

Replace `$(SolutionDir)` with an anchored path in `game/game/game.vcxproj` so the project builds from any entry point:

```xml
<IncludePath>$(MSBuildThisFileDirectory)..\..\shared;$(MSBuildThisFileDirectory)..\..\deps\dks\Include;$(IncludePath)</IncludePath>
<LibraryPath>$(MSBuildThisFileDirectory)..\..\deps\dks\Lib\x86;$(LibraryPath)</LibraryPath>
<OutDir>$(MSBuildThisFileDirectory)..\..\build\bin\Game\</OutDir>
```

This fixes causes #2, #3, and the outside-the-repo `OutDir` in one edit.

**3. Point Debug at the vendored SDK too**, removing the `DXSDK_DIR` dependency, and delete the `C:\Pristontale EU\` xcopy step.

**4. Add `Release|Win32` to `PristonTale.sln`**, or document that solution builds require `/p:Platform=x86`.

**5. Add a `game` target to `CMakeLists.txt`.** Currently CMake builds only `server`. A client target is straightforward since all sources and the SDK are vendored:

```cmake
file(GLOB GAME_CPP "${CMAKE_CURRENT_SOURCE_DIR}/game/game/*.cpp")
# NOTE: must exclude the 31 orphaned files listed above
add_library(game SHARED ${GAME_CPP} ${SHARED_GAME})
target_include_directories(game PRIVATE ${SHARED_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/deps/dks/Include")
target_link_directories(game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/dks/Lib/x86")
target_compile_definitions(game PRIVATE _GAME;WIN32;_WINDOWS;_USRDLL;GAME_EXPORTS)
target_precompile_headers(game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/game/game/stdafx.h")
target_link_options(game PRIVATE /DEF:"${CMAKE_CURRENT_SOURCE_DIR}/game/game/export.def")
set_target_properties(game PROPERTIES PREFIX "" SUFFIX ".dll" OUTPUT_NAME "game")
```

**6. Fix or delete `game/game/PropertySheet.props`** and consider deleting the broken `game/game.sln`.

### Critical deployment warning

Because of the ~4,000 hardcoded addresses, a freshly built `game.dll` is bound to one exact `Game.exe`. Before replacing `Files/Game/game.dll`:

- back up the working DLL
- remember the shipped one was **patched at the byte level** to `127.0.0.1` (see `docs/guides/client-localhost-patch-guide.md`); a fresh build takes its IP from `game/game/globals.h`, which already reads `127.0.0.1`, so re-patching should be unnecessary — but verify
- if the source tree has drifted from the `Game.exe` the addresses were derived from, the new DLL will crash rather than fail gracefully

---

## Part 3: Serverless single-EXE feasibility

### Answer

**Not possible as a modification of this codebase.** Possible only as a from-scratch reimplementation of the server's simulation inside the client process — realistically a multi-year effort, and the result would no longer be this codebase.

The instinct is that "no server" means removing the network layer. That is the easy 5%. The real problem is that the game's *rules* are not in the client.

### Why: the client is a renderer, the server is the game

The server is not a relay. It is a full authoritative simulation of comparable size to the client:

- **155 files**, ~**4.35 MB** of source, ~**108,000 lines**

And it owns essentially everything that constitutes gameplay:

| Authoritative system | Server implementation | Size |
|---|---|---|
| Items, stats, generation | `itemserver.cpp` | 9,389 lines |
| GM/admin commands | `servercommand.cpp` | 6,183 lines |
| Quests | `questserver.cpp` | 4,734 lines |
| Combat / damage | `DamageHandler.cpp` | 3,923 lines |
| Maps, zones, spawns | `mapserver.cpp` | 4,138 lines |
| Monster/NPC units, AI ticks | `unitserver.cpp` | 3,597 lines |
| Characters, EXP, leveling | `characterserver.cpp` | 2,899 lines |
| Accounts, persistence | `userserver.cpp` | ~2,780 lines |
| Skills | `HNSSkill.cpp` + 8 per-class files | 2,299 + ~2,000 lines |
| Drops / loot | `lootserver.cpp` | — |
| Trade, warehouse, mixing | `TradeHandler.cpp`, `CWarehouseHandler.cpp`, `MixHandler.cpp` | ~2,000+ lines |

Monster AI genuinely ticks server-side — `Server/server/unitserver.cpp:3106`:

```cpp
void UnitServer::Update()
{
	//Only GameServers
	if ( !GAME_SERVER )
		return;
	...
	for ( UINT i = 0; i < MAX_UNITS; i++ )
	{
		...
		if ( pcUnitData->bActive )
		{
			//Frame Check
			if ( (i % 4) == (iUnitWheel % 4) )
				UpdateUnit( pcUnit );   // AI, movement, aggro, attacks
		}
	}
}
```

The client/server split is formalized in the data model itself. From `shared/unit.h:166`:

```cpp
enum EActionPattern : int
{
	ACTIONMODE_ClientSelf     = 0,   //Main character
	ACTIONMODE_ServerMonster  = 5,   //Monster
	ACTIONMODE_ServerNPC      = 12,  //Npc
	ACTIONMODE_ClientUnit     = 99,  //Other client unit
	ACTIONMODE_ClientTarget   = 101,
};
```

Every monster and NPC is explicitly server-driven. The client only interpolates and renders what it is told.

### The four blockers, in order of severity

**Blocker 1 — the missing logic is in a binary you cannot recompile.**

This is the decisive one. Even the *server's* logic is split between `server.dll` (which you have) and `Server.exe` (which you do not). `Server/server/unitserver.cpp` patches `Server.exe` memory directly and calls into it. So "move the server logic into the client" is not a code-motion task — a portion of that logic exists only as machine code inside a closed 6.4 MB binary. You would have to reverse-engineer and reimplement it.

Worse, the honesty of the source confirms parts are not even understood by the current maintainers. `Server/server/unitserver.cpp:3220`:

```cpp
//Line 5012 in leaked source code
//character.cpp - int smCHAR::EventAttack(int Flag) case 5:	//monster
//ActionPattern = 5 = UPDATEMODE_ServerMonster
//This code doesn't work at the moment. The monster doesn't
//attack players.
BOOL UnitServer::MainUnitMonsterData( UnitData * pcUnitData )
```

Monster attack logic is *already partially broken* in the code you do have.

**Blocker 2 — the protocol is the API, and it is enormous.**

- **381** distinct packet identifiers (`PKTHDR_*`) in `shared/packets.h`
- **200** packet structs, across 2,968 lines
- **156** dispatch cases in the client's `game/game/RecvPacket.cpp` alone

The client's state machine is driven entirely by inbound packets. To go serverless you must synthesize correct responses for all of them, in the right order, with the right timing. The client is written to *react*, never to decide.

**Blocker 3 — persistence is SQL Server, not files.**

Ten databases (`GameDB`, `UserDB`, `ServerDB`, `LogDB`, `SkillDB`, `SkillDBNew`, `EventDB`, `ItemDB`, `ClanDB`, `ChatDB`) hold both static content (monster stats, drop tables, NPC data, quest definitions, mixing recipes) and live state (characters, inventory, warehouse, quest progress). Accessed through ODBC in `Server/server/SQLConnection.cpp` (889 lines).

"No server" would still mean either shipping SQL Server, or migrating all ten schemas to an embedded store (SQLite) and rewriting every query. Note `docs/analysis/project-analysis.md` already records that `ChatDB`, `SkillDB` and `SoD2DB` are **absent** from the runtime pack — so a complete dataset is not even on hand.

**Blocker 4 — the client is hardwired for two live sockets.**

`game/game/SocketGame.h` defines `MAX_CONNECTIONS 2` with separate login (`10009`) and game (`10007`) sockets, background `Receiver`/`Sender` threads, ping tracking, disconnect codes, and reconnect counters. `game/game/CGameWorld.cpp:12` resolves server IPs at init. Anti-cheat (`AntiCheat.cpp`, 28.59 KB), anti-debug (`CAntiDebuggerHandler.cpp`) and the `LdrLoadDll` anti-injection hook in `Main.cpp` all assume a hostile-client / trusted-server model that becomes meaningless — and actively obstructive — offline.

### Options, honestly assessed

| Option | Effort | Verdict |
|---|---|---|
| **A. Loopback bundle** — keep `Server.exe` + `loginserver.exe` + SQL, hide them behind a launcher that starts them silently | Days | **Recommended.** Delivers the actual user-facing goal ("double-click and play, no setup") with near-zero risk. Not literally serverless, but functionally indistinguishable to the player. |
| **B. Embedded server thread** — start the server logic inside the client process, still speaking packets over loopback | Weeks–months | Possible but ugly. Both DLLs patch *different* prebuilt EXEs at fixed addresses; you cannot host both address spaces in one process. Requires a real port off `Server.exe`. |
| **C. Local emulator** — replace the socket layer with an in-process fake that answers all 381 packet types | Many months | Feasible for a narrow demo (log in, walk around, no combat/loot/quests). Becomes Option D the moment you want real gameplay. |
| **D. Full reimplementation** — rewrite the authoritative simulation client-side, migrate to SQLite | Years | Only true path to a genuine single `game.exe`. At that point it is a new game engine reusing PT assets, not this repository. |
| **E. Compile a real `game.exe`** | N/A | **Impossible.** No source exists for the EXE. |

### Recommendation

Pursue **Option A**. Concretely:

1. Write a small launcher that starts `loginserver`, `gameserver`, and the SQL instance as hidden child processes, waits for readiness, launches `Game.exe`, and tears everything down on exit.
2. Replace SQL Server in Docker with **SQL Server Express LocalDB** to remove the Docker prerequisite.
3. Reuse the existing `scripts/*.ps1` logic (`start-pt-server.ps1`, `restore-pt-docker-dbs.ps1`, `fix-pt-local-runtime.ps1`) as the launcher's first-run provisioning step.

The player experiences one icon and no server. You avoid touching 108,000 lines of authoritative logic and two binaries you cannot rebuild.

If the true requirement is literally "one process, no sockets, no SQL," then the correct framing is that you are **writing a new server**, and the decision to make is build-vs-abandon — not port.

---

## Verification appendix

Environment used for all builds in this study:

| Item | Value |
|---|---|
| Visual Studio | `C:\Program Files\Microsoft Visual Studio\18\Community` |
| MSVC toolsets available | `14.29.30133`, `14.44.35207`, `14.51.36231` |
| Platform toolsets available | `v150`, `v160`, `v170`, `v180` |
| `DXSDK_DIR` | **not set** |
| CMake | `C:\Program Files\CMake\bin\cmake.exe` |
| Target platform | Win32 / x86 (mandatory — the code is 32-bit only, with inline `__asm`) |

Checks performed:

- project-vs-disk file reconciliation for both `ClCompile` and `ClInclude` → 0 missing
- five distinct build invocations (matrix above) → 1 success, 4 failures, each root-caused
- `dumpbin /EXPORTS` on the built DLL → confirms `WinMain = _WinMain@16`
- binary string scan of `Files/Game/Game.exe` → confirms it imports `game.dll` / `WinMain`
- address-density scan → 3,997 hardcoded addresses in `game/game`, 42 server files with `0x004xxxxx`-class addresses
- protocol scale → 381 `PKTHDR_*` identifiers, 200 structs, 156 client dispatch cases

### Related documentation

- `docs/analysis/project-analysis.md` — runtime pack analysis, source/runtime mismatch
- `docs/guides/client-localhost-patch-guide.md` — the byte-level `game.dll` IP patch
- `docs/guides/server-start-guide.md` — current server startup procedure
- `docs/troubleshooting/local-runtime-known-issues.md` — known runtime issues
