[CmdletBinding()]
param(
    [string]$SqlServer = '127.0.0.1,1433',
    [string]$SqlUser = 'sa',
    [string]$SqlPassword = '632514Go'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Data

$repoRoot = Split-Path -Parent $PSScriptRoot
$outputPath = Join-Path $repoRoot 'docs\reference\monster-spawn-reference.md'

$connStr = "Server=$SqlServer;User ID=$SqlUser;Password=$SqlPassword;Encrypt=False;TrustServerCertificate=True;Database=GameDB"
$conn = New-Object System.Data.SqlClient.SqlConnection $connStr

Write-Host "Connecting to database..." -ForegroundColor Cyan
$conn.Open()

# -- Fetch all maps --
Write-Host "Fetching maps..." -ForegroundColor DarkGray
$cmdMaps = $conn.CreateCommand()
$cmdMaps.CommandTimeout = 0
$cmdMaps.CommandText = "SELECT ID, Name, TypeMap, LevelReq FROM dbo.MapList ORDER BY ID"
$daMaps = New-Object System.Data.SqlClient.SqlDataAdapter $cmdMaps
$maps = New-Object System.Data.DataTable
[void]$daMaps.Fill($maps)

$mapLookup = @{}
foreach ($row in $maps.Rows) {
    $mapLookup[[int]$row['ID']] = @{
        Name = $row['Name']; Type = $row['TypeMap']; LevelReq = [int]$row['LevelReq']
    }
}

# -- Fetch all MapMonster rows --
Write-Host "Fetching map-monster data..." -ForegroundColor DarkGray
$cmdMM = $conn.CreateCommand()
$cmdMM.CommandTimeout = 0
$cmdMM.CommandText = @"
SELECT Stage, MaxMonsters,
    Monster1,Count1, Monster2,Count2, Monster3,Count3, Monster4,Count4,
    Monster5,Count5, Monster6,Count6, Monster7,Count7, Monster8,Count8,
    Monster9,Count9, Monster10,Count10, Monster11,Count11, Monster12,Count12,
    BossMonster1, BossMonster2, BossMonster3
FROM dbo.MapMonster ORDER BY Stage
"@
$daMM = New-Object System.Data.SqlClient.SqlDataAdapter $cmdMM
$mapMonsters = New-Object System.Data.DataTable
[void]$daMM.Fill($mapMonsters)

# -- Fetch all monsters --
Write-Host "Fetching monster data..." -ForegroundColor DarkGray
$cmdMon = $conn.CreateCommand()
$cmdMon.CommandTimeout = 0
$cmdMon.CommandText = "SELECT ID, Name, Level, HP, ATKPowMin, ATKPowMax, Defense, Absorb, EXP, MonsterType FROM dbo.MonsterList ORDER BY Level, Name"
$daMon = New-Object System.Data.SqlClient.SqlDataAdapter $cmdMon
$monsters = New-Object System.Data.DataTable
[void]$daMon.Fill($monsters)

$conn.Close()

# Build monster lookup: name -> level
$monsterLvlLookup = @{}
foreach ($row in $monsters.Rows) {
    $name = $row['Name'].ToString().Trim()
    if ($name) { $monsterLvlLookup[$name] = [int]$row['Level'] }
}

# Build zone level as median of monster levels in each zone
function Get-Median([int[]]$values) {
    if (-not $values -or $values.Count -eq 0) { return $null }
    $sorted = $values | Sort-Object
    $n = $sorted.Count
    if ($n % 2 -eq 1) { return $sorted[($n - 1) / 2] }
    else { return [Math]::Floor(($sorted[$n / 2 - 1] + $sorted[$n / 2]) / 2) }
}

Write-Host "Computing zone levels..." -ForegroundColor DarkGray
$zoneLevels = @{}
foreach ($mmRow in $mapMonsters.Rows) {
    $stage = [int]$mmRow['Stage']
    $levels = [System.Collections.Generic.List[int]]::new()
    for ($i = 1; $i -le 12; $i++) {
        $mName = $mmRow["Monster$i"]
        if ($mName -and $mName.ToString().Trim()) {
            $ml = $monsterLvlLookup[$mName.ToString().Trim()]
            if ($ml -ne $null) { [void]$levels.Add($ml) }
        }
    }
    for ($i = 1; $i -le 3; $i++) {
        $bName = $mmRow["BossMonster$i"]
        if ($bName -and $bName.ToString().Trim()) {
            $bl = $monsterLvlLookup[$bName.ToString().Trim()]
            if ($bl -ne $null) { [void]$levels.Add($bl) }
        }
    }
    $zoneLevels[$stage] = Get-Median $levels.ToArray()
}

# Build monster -> zones mapping
Write-Host "Building monster-zone lookup..." -ForegroundColor DarkGray
$monsterZones = @{}

foreach ($mmRow in $mapMonsters.Rows) {
    $stage = [int]$mmRow['Stage']
    $mapInfo = $mapLookup[$stage]
    if (-not $mapInfo) { continue }
    $zoneLvl = $zoneLevels[$stage]

    for ($i = 1; $i -le 12; $i++) {
        $mName = $mmRow["Monster$i"]
        if ($mName -and $mName.ToString().Trim()) {
            $mNameClean = $mName.ToString().Trim()
            if (-not $monsterZones[$mNameClean]) { $monsterZones[$mNameClean] = @() }
            $monsterZones[$mNameClean] += [PSCustomObject]@{
                MapName = $mapInfo.Name; MapLevel = $zoneLvl; Count = $mmRow["Count$i"]; IsBoss = $false
            }
        }
    }
    for ($i = 1; $i -le 3; $i++) {
        $bName = $mmRow["BossMonster$i"]
        if ($bName -and $bName.ToString().Trim()) {
            $bNameClean = $bName.ToString().Trim()
            if (-not $monsterZones[$bNameClean]) { $monsterZones[$bNameClean] = @() }
            $monsterZones[$bNameClean] += [PSCustomObject]@{
                MapName = $mapInfo.Name; MapLevel = $zoneLvl; Count = '-'; IsBoss = $true
            }
        }
    }
}

# Split monsters: with spawns vs without
$monstersWithZones = @()
$monstersWithoutZones = @()
foreach ($row in $monsters.Rows) {
    $monName = $row['Name'].ToString().Trim()
    if ($monsterZones[$monName]) { $monstersWithZones += $row }
    else { $monstersWithoutZones += $row }
}

Write-Host "Generating markdown..." -ForegroundColor Cyan
$sb = New-Object System.Text.StringBuilder

[void]$sb.AppendLine('# Monster Spawn Reference')
[void]$sb.AppendLine()
[void]$sb.AppendLine('> Auto-generated from `GameDB.dbo.MonsterList`, `GameDB.dbo.MapMonster`, and `GameDB.dbo.MapList`')
[void]$sb.AppendLine("> Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
[void]$sb.AppendLine()
[void]$sb.AppendLine('> **Zone Level** = median of all monster levels in the zone (same as zone-reference.md).')
[void]$sb.AppendLine()

# -- Section 1: Monsters with Zone Spawns --
[void]$sb.AppendLine('## Monsters with Zone Spawns')
[void]$sb.AppendLine()
[void]$sb.AppendLine("$($monstersWithZones.Count) monsters ($([Math]::Round($monstersWithZones.Count / $monsters.Rows.Count * 100, 1))% of total)")
[void]$sb.AppendLine()
[void]$sb.AppendLine('| Lvl | Monster | ID | Type | HP | ATK | DEF | Zones (sorted by zone level) |')
[void]$sb.AppendLine('| --- | --- | --- | --- | --- | --- | --- | --- |')

foreach ($row in $monstersWithZones) {
    $monName = $row['Name'].ToString().Trim()
    $monId = $row['ID']; $monLvl = $row['Level']; $monHP = $row['HP']
    $monAtkMin = $row['ATKPowMin']; $monAtkMax = $row['ATKPowMax']
    $monDef = $row['Defense']; $monType = $row['MonsterType']
    $atkStr = if ($monAtkMin -and $monAtkMax) { "$monAtkMin-$monAtkMax" } else { '-' }

    $zoneList = $monsterZones[$monName]
    $sortedZones = $zoneList | Sort-Object { if ($_.MapLevel -ne $null) { $_.MapLevel } else { 999 } }, MapName
    $zoneStrs = foreach ($z in $sortedZones) {
        $prefix = if ($z.IsBoss) { "Boss@" } else { "" }
        $zlvl = if ($z.MapLevel -ne $null) { "$($z.MapLevel)" } else { '?' }
        "$prefix$($z.MapName)($zlvl)"
    }
    [void]$sb.AppendLine("| $monLvl | $monName | $monId | $monType | $monHP | $atkStr | $monDef | $($zoneStrs -join ', ') |")
}
[void]$sb.AppendLine()

# -- Section 2: Monsters without Zone Spawns --
[void]$sb.AppendLine('## Monsters without Zone Spawns')
[void]$sb.AppendLine()
[void]$sb.AppendLine("$($monstersWithoutZones.Count) monsters -- these exist in `MonsterList` but are not assigned to any map in `MapMonster`.")
[void]$sb.AppendLine('They may be event monsters, summons, crystals, or unused entries.')
[void]$sb.AppendLine()
[void]$sb.AppendLine('| Lvl | Monster | ID | Type | HP | ATK | DEF |')
[void]$sb.AppendLine('| --- | --- | --- | --- | --- | --- | --- |')

foreach ($row in $monstersWithoutZones) {
    $monName = $row['Name'].ToString().Trim()
    $monId = $row['ID']; $monLvl = $row['Level']; $monHP = $row['HP']
    $monAtkMin = $row['ATKPowMin']; $monAtkMax = $row['ATKPowMax']
    $monDef = $row['Defense']; $monType = $row['MonsterType']
    $atkStr = if ($monAtkMin -and $monAtkMax) { "$monAtkMin-$monAtkMax" } else { '-' }
    [void]$sb.AppendLine("| $monLvl | $monName | $monId | $monType | $monHP | $atkStr | $monDef |")
}
[void]$sb.AppendLine()

$sb.ToString() | Out-File -FilePath $outputPath -Encoding utf8

Write-Host "Done! $($monstersWithZones.Count) with spawns, $($monstersWithoutZones.Count) without." -ForegroundColor Green
Write-Host "Output written to: $outputPath" -ForegroundColor Green
