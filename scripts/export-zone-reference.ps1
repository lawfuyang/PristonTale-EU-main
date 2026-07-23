[CmdletBinding()]
param(
    [string]$SqlServer = '127.0.0.1,1433',
    [string]$SqlUser = 'sa',
    [string]$SqlPassword = '632514Go'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Data

$repoRoot = Split-Path -Parent $PSScriptRoot
$outputPath = Join-Path $repoRoot 'docs\reference\zone-reference.md'

$connStr = "Server=$SqlServer;User ID=$SqlUser;Password=$SqlPassword;Encrypt=False;TrustServerCertificate=True;Database=GameDB"
$conn = New-Object System.Data.SqlClient.SqlConnection $connStr

Write-Host "Connecting to database..." -ForegroundColor Cyan
$conn.Open()

$cmdMaps = $conn.CreateCommand()
$cmdMaps.CommandTimeout = 0
$cmdMaps.CommandText = "SELECT ID, Name, ShortName, TypeMap, LevelReq, StageFile FROM dbo.MapList ORDER BY ID"
$daMaps = New-Object System.Data.SqlClient.SqlDataAdapter $cmdMaps
$maps = New-Object System.Data.DataTable
[void]$daMaps.Fill($maps)

Write-Host "Fetching map-monster data..." -ForegroundColor DarkGray
$cmdMM = $conn.CreateCommand()
$cmdMM.CommandTimeout = 0
$cmdMM.CommandText = @"
SELECT Stage, MaxMonsters, Interval,
    Monster1,Count1, Monster2,Count2, Monster3,Count3, Monster4,Count4,
    Monster5,Count5, Monster6,Count6, Monster7,Count7, Monster8,Count8,
    Monster9,Count9, Monster10,Count10, Monster11,Count11, Monster12,Count12,
    BossMonster1, HoursBossMonster1, BossMonster2, HoursBossMonster2, BossMonster3, HoursBossMonster3,
    SubMonster1, CountSub1, SubMonster2, CountSub2, SubMonster3, CountSub3
FROM dbo.MapMonster ORDER BY Stage
"@
$daMM = New-Object System.Data.SqlClient.SqlDataAdapter $cmdMM
$mapMonsters = New-Object System.Data.DataTable
[void]$daMM.Fill($mapMonsters)

Write-Host "Fetching monster data..." -ForegroundColor DarkGray
$cmdMon = $conn.CreateCommand()
$cmdMon.CommandTimeout = 0
$cmdMon.CommandText = "SELECT ID, Name, Level, HP, ATKPowMin, ATKPowMax, Defense, Absorb, EXP FROM dbo.MonsterList ORDER BY Level, Name"
$daMon = New-Object System.Data.SqlClient.SqlDataAdapter $cmdMon
$monsters = New-Object System.Data.DataTable
[void]$daMon.Fill($monsters)

$conn.Close()

# Build monster name -> data lookup
Write-Host "Building lookups..." -ForegroundColor DarkGray
$monsterLookup = @{}
foreach ($row in $monsters.Rows) {
    $name = $row['Name']
    if ($name -and $name.ToString().Trim()) {
        $monsterLookup[$name] = @{
            Level = $row['Level']; HP = $row['HP']
            MinAtk = $row['ATKPowMin']; MaxAtk = $row['ATKPowMax']
            Defense = $row['Defense']; Absorb = $row['Absorb']; EXP = $row['EXP']
        }
    }
}

$mmLookup = @{}
foreach ($row in $mapMonsters.Rows) {
    $mmLookup[[int]$row['Stage']] = $row
}

function Get-Median([int[]]$values) {
    if (-not $values -or $values.Count -eq 0) { return $null }
    $sorted = $values | Sort-Object
    $n = $sorted.Count
    if ($n % 2 -eq 1) { return $sorted[($n - 1) / 2] }
    else { return [Math]::Floor(($sorted[$n / 2 - 1] + $sorted[$n / 2]) / 2) }
}

# Pre-compute zone levels (median monster level) and monster lists
Write-Host "Computing zone levels..." -ForegroundColor DarkGray
$zoneLevels = @{}
$zoneMonsterList = @{}

foreach ($map in $maps.Rows) {
    $mapId = [int]$map['ID']
    $mmRow = $mmLookup[$mapId]
    $levels = [System.Collections.Generic.List[int]]::new()
    $monsterInfos = @()

    if ($mmRow) {
        for ($i = 1; $i -le 12; $i++) {
            $mName = $mmRow["Monster$i"]
            if ($mName -and $mName.ToString().Trim()) {
                $mNameClean = $mName.ToString().Trim()
                $ml = $monsterLookup[$mNameClean]
                $mlvl = if ($ml) { [int]$ml.Level } else { $null }
                if ($mlvl -ne $null) { [void]$levels.Add($mlvl) }
                $monsterInfos += [PSCustomObject]@{ Name = $mNameClean; Level = $mlvl; IsBoss = $false }
            }
        }
        for ($i = 1; $i -le 3; $i++) {
            $bName = $mmRow["BossMonster$i"]
            if ($bName -and $bName.ToString().Trim()) {
                $bNameClean = $bName.ToString().Trim()
                $bl = $monsterLookup[$bNameClean]
                $blvl = if ($bl) { [int]$bl.Level } else { $null }
                if ($blvl -ne $null) { [void]$levels.Add($blvl) }
                $monsterInfos += [PSCustomObject]@{ Name = $bNameClean; Level = $blvl; IsBoss = $true }
            }
        }
    }

    $zoneLevels[$mapId] = Get-Median $levels.ToArray()
    $zoneMonsterList[$mapId] = $monsterInfos
}

# Split maps
$zonesWithMonsters = @()
$zonesWithoutMonsters = @()
foreach ($map in $maps.Rows) {
    if ($zoneMonsterList[[int]$map['ID']].Count -gt 0) { $zonesWithMonsters += $map }
    else { $zonesWithoutMonsters += $map }
}

Write-Host "Generating markdown..." -ForegroundColor Cyan

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('# Zone & Monster Reference')
[void]$sb.AppendLine()
[void]$sb.AppendLine('> Auto-generated from `GameDB.dbo.MapList`, `GameDB.dbo.MapMonster`, and `GameDB.dbo.MonsterList`')
[void]$sb.AppendLine("> Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
[void]$sb.AppendLine()
[void]$sb.AppendLine('> **Zone Level** = median of all monster levels that spawn in the zone.')
[void]$sb.AppendLine()

# -- Zones with Monsters --
[void]$sb.AppendLine('## Zones with Monsters')
[void]$sb.AppendLine()
$sortedWith = $zonesWithMonsters | Sort-Object { $zoneLevels[[int]$_.ID] }, { $_.Name }

[void]$sb.AppendLine('| Zone | ID | Type | Zone Lvl | Monsters |')
[void]$sb.AppendLine('| --- | --- | --- | --- | --- |')

foreach ($map in $sortedWith) {
    $mapId = [int]$map['ID']
    $mapName = $map['Name']; $mapType = $map['TypeMap']
    $zoneLvl = $zoneLevels[$mapId]
    $zoneLvlStr = if ($zoneLvl -ne $null) { "$zoneLvl" } else { '?' }
    $infos = $zoneMonsterList[$mapId]

    $monsterStrs = foreach ($info in ($infos | Sort-Object Level, Name)) {
        $lvlStr = if ($info.Level -ne $null) { "$($info.Level)" } else { '?' }
        if ($info.IsBoss) { "**Boss:** $($info.Name) ($lvlStr)" } else { "$($info.Name) ($lvlStr)" }
    }
    [void]$sb.AppendLine("| $mapName | $mapId | $mapType | $zoneLvlStr | $($monsterStrs -join ', ') |")
}
[void]$sb.AppendLine()

# -- Zones without Monsters --
[void]$sb.AppendLine('## Zones without Monsters')
[void]$sb.AppendLine()
$sortedWithout = $zonesWithoutMonsters | Sort-Object { [int]$_.LevelReq }, { $_.Name }

[void]$sb.AppendLine('| Zone | ID | Type | Req Lvl |')
[void]$sb.AppendLine('| --- | --- | --- | --- |')

foreach ($map in $sortedWithout) {
    [void]$sb.AppendLine("| $($map['Name']) | $($map['ID']) | $($map['TypeMap']) | $($map['LevelReq']) |")
}
[void]$sb.AppendLine()

# -- Zone Details (with monsters only) --
[void]$sb.AppendLine('## Zone Details')
[void]$sb.AppendLine()

foreach ($map in $sortedWith) {
    $mapId = [int]$map['ID']
    $mapName = $map['Name']; $mapType = $map['TypeMap']
    $levelReq = $map['LevelReq']; $stageFile = $map['StageFile']
    $zoneLvl = $zoneLevels[$mapId]
    $zoneLvlStr = if ($zoneLvl -ne $null) { "$zoneLvl" } else { '?' }

    [void]$sb.AppendLine("### $mapName (ID: $mapId)")
    [void]$sb.AppendLine()
    [void]$sb.AppendLine("- **Type:** $mapType | **Req Level:** $levelReq | **Zone Level (median):** $zoneLvlStr | **Stage File:** $stageFile")
    [void]$sb.AppendLine()

    $mmRow = $mmLookup[$mapId]
    if ($mmRow) {
        [void]$sb.AppendLine("- **Max Monsters:** $($mmRow['MaxMonsters']) | **Spawn Interval:** $($mmRow['Interval'])s")
        [void]$sb.AppendLine()

        # Regular monsters
        $monsterRows = @()
        for ($i = 1; $i -le 12; $i++) {
            $mName = $mmRow["Monster$i"]
            if ($mName -and $mName.ToString().Trim()) {
                $mNameClean = $mName.ToString().Trim()
                $ml = $monsterLookup[$mNameClean]
                $monsterRows += [PSCustomObject]@{
                    Name = $mNameClean; Count = $mmRow["Count$i"]
                    Level = if ($ml) { $ml.Level } else { '?' }
                    HP = if ($ml) { $ml.HP } else { '?' }
                    ATK = if ($ml -and $ml.MinAtk -and $ml.MaxAtk) { "$($ml.MinAtk)-$($ml.MaxAtk)" } else { '?' }
                    DEF = if ($ml) { $ml.Defense } else { '?' }
                    ABS = if ($ml) { $ml.Absorb } else { '?' }
                    EXP = if ($ml) { $ml.EXP } else { '?' }
                }
            }
        }

        if ($monsterRows.Count -gt 0) {
            [void]$sb.AppendLine('| Monster | Count | Level | HP | ATK | DEF | ABS | EXP |')
            [void]$sb.AppendLine('| --- | --- | --- | --- | --- | --- | --- | --- |')
            $monsterRows = $monsterRows | Sort-Object { if ($_.Level -is [int]) { $_.Level } else { 999 } }
            foreach ($mr in $monsterRows) {
                [void]$sb.AppendLine("| $($mr.Name) | $($mr.Count) | $($mr.Level) | $($mr.HP) | $($mr.ATK) | $($mr.DEF) | $($mr.ABS) | $($mr.EXP) |")
            }
            [void]$sb.AppendLine()
        }

        # Bosses
        $bossRows = @()
        for ($i = 1; $i -le 3; $i++) {
            $bName = $mmRow["BossMonster$i"]
            if ($bName -and $bName.ToString().Trim()) {
                $bNameClean = $bName.ToString().Trim()
                $bl = $monsterLookup[$bNameClean]
                $bSub = $mmRow["SubMonster$i"]
                $bossRows += [PSCustomObject]@{
                    Name = $bNameClean
                    Hours = $mmRow["HoursBossMonster$i"]
                    Sub = if ($bSub -and $bSub.ToString().Trim()) { $bSub.ToString().Trim() } else { '-' }
                    SubCount = $mmRow["CountSub$i"]
                    Level = if ($bl) { $bl.Level } else { '?' }
                    HP = if ($bl) { $bl.HP } else { '?' }
                }
            }
        }

        if ($bossRows.Count -gt 0) {
            [void]$sb.AppendLine('**Bosses:**')
            [void]$sb.AppendLine()
            [void]$sb.AppendLine('| Boss | Hours | Sub Monsters | Sub Count | Level | HP |')
            [void]$sb.AppendLine('| --- | --- | --- | --- | --- | --- |')
            foreach ($br in $bossRows) {
                [void]$sb.AppendLine("| **$($br.Name)** | $($br.Hours) | $($br.Sub) | $($br.SubCount) | $($br.Level) | $($br.HP) |")
            }
            [void]$sb.AppendLine()
        }
    }
}

$sb.ToString() | Out-File -FilePath $outputPath -Encoding utf8

Write-Host "Done! $($zonesWithMonsters.Count) zones with monsters, $($zonesWithoutMonsters.Count) zones without." -ForegroundColor Green
Write-Host "Output written to: $outputPath" -ForegroundColor Green
