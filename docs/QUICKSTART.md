# Quick-Start: Run Priston Tale Fully Locally

This is the shortest working path to get the game running on a single Windows machine.

## Prerequisites

- **Docker Desktop** installed and running
- **The `Files/` runtime pack** at `D:\Workspace\PristonTale-EU\Files\` (already present)
- **PowerShell 5.1+** (built into Windows)

No compilation needed — everything is pre-built.

---

## One-Time Setup

Run these **once**, in order. All commands run from the repository root:

```powershell
cd D:\Workspace\PristonTale-EU
```

### Step 1 — Extract database backups

The zip files in `Files\DBS\` need to be extracted so the restore script can find the `.bak` files:

```powershell
New-Item -ItemType Directory -Path '.\Files\DBS\extracted' -Force

Get-ChildItem '.\Files\DBS\*.zip' | ForEach-Object {
    Expand-Archive -Path $_.FullName -DestinationPath '.\Files\DBS\extracted' -Force
}

Move-Item '.\Files\DBS\UserDB202209251906.bak' '.\Files\DBS\extracted\' -Force -ErrorAction SilentlyContinue
```

### Step 2 — Start SQL Server in Docker

```powershell
.\scripts\start-pt-docker-sql.ps1
```

This creates a `priston-sql` container with SQL Server 2022, exposed on `127.0.0.1:1433`. Wait for the "SQL Server do Docker esta pronto" message.

### Step 3 — Restore all databases

```powershell
.\scripts\restore-pt-docker-dbs.ps1
```

This restores 8 databases from the `.bak` files and creates the `admin/admin` test account automatically.

### Step 4 — Patch the client to localhost

```powershell
.\scripts\patch-pt-client-localhost.ps1
```

The pre-compiled `game.dll` shipped pointing to a public IP. This patches it to `127.0.0.1`. A backup is saved as `game.dll.bak`.

### Step 5 — Apply local runtime fixes

```powershell
.\scripts\fix-pt-local-runtime.ps1
```

Fixes known issues: reduces `Administrador` gold (avoids cheat warning), cleans up broken timer data, and binds all test characters to the `admin` account.

---

## Start the Game

### Start both servers + open the client

```powershell
.\scripts\start-pt-server.ps1 -OpenClient
```

Two PowerShell monitor windows open (login server + game server), then the game launches.

### Or, start servers and client separately

```powershell
.\scripts\start-pt-server.ps1        # Opens two monitoring windows
.\Files\Game\Game.exe                # Launch the game client
```

---

## Login

| Field | Value |
|-------|-------|
| Username | `admin` |
| Password | `admin` |

Select the `Administrador` character. Once in-game, open chat and type:

```
/activategm
```

You now have full GM4/Admin access.

---

## Shut Down

```powershell
.\scripts\stop-pt-server.ps1         # Kills both servers + monitor windows
.\scripts\stop-pt-docker-sql.ps1     # Stops the SQL container
```

---

## Next Time You Want to Play

If the SQL container is still running from last time, just do:

```powershell
.\scripts\start-pt-server.ps1 -OpenClient
```

If Docker was restarted, start from Step 2.

---

## Quick Reference

| Need | Command |
|------|---------|
| Spawn an item | `/getitem wa131` (Abyss Axe) |
| Add gold | `/GetGold 1000000` |
| Level up to 100 | `/!levelup 100` |
| Teleport to map | `/wrap 3 0 0` |
| Double EXP | `/expevent 100` |
| Find item codes | `.\scripts\find-pt-item.ps1 -Search "axe"` |
| Find map IDs | `.\scripts\find-pt-map.ps1 -Search "ricarten"` |

For the full GM command list, see `docs\reference\server-commands-reference.md`.
For item/map/monster ID lookups, see `docs\reference\ids\`.
