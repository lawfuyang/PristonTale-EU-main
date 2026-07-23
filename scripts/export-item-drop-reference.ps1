[CmdletBinding()]
param(
    [string]$SqlServer = '127.0.0.1,1433',
    [string]$SqlUser = 'sa',
    [string]$SqlPassword = '632514Go'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Data

$repoRoot = Split-Path -Parent $PSScriptRoot
$outputPath = Join-Path $repoRoot 'docs\reference\item-drop-reference.md'

$connStr = "Server=$SqlServer;User ID=$SqlUser;Password=$SqlPassword;Encrypt=False;TrustServerCertificate=True;Database=GameDB"
$conn = New-Object System.Data.SqlClient.SqlConnection $connStr

Write-Host "Connecting to database..." -ForegroundColor Cyan
$conn.Open()

# -- Fetch all monsters --
Write-Host "Fetching monsters..." -ForegroundColor DarkGray
$cmdMon = $conn.CreateCommand()
$cmdMon.CommandTimeout = 0
$cmdMon.CommandText = "SELECT ID, Name, Level, MonsterID FROM dbo.MonsterList ORDER BY Level, Name"
$daMon = New-Object System.Data.SqlClient.SqlDataAdapter $cmdMon
$monsters = New-Object System.Data.DataTable
[void]$daMon.Fill($monsters)

# Build monster lookup: MonsterID -> {Name, Level}
# DropItem.DropID maps to MonsterList.MonsterID (NOT MonsterList.ID)
$monLookup = @{}
foreach ($row in $monsters.Rows) {
    $mid = [int]$row['MonsterID']
    if ($mid -gt 0) {
        $monLookup[$mid] = @{ Name = $row['Name']; Level = [int]$row['Level'] }
    }
}

# Build boss name set from MapMonster.BossMonster1-3
Write-Host "Fetching boss names..." -ForegroundColor DarkGray
$cmdBoss = $conn.CreateCommand()
$cmdBoss.CommandTimeout = 0
$cmdBoss.CommandText = "SELECT BossMonster1, BossMonster2, BossMonster3 FROM dbo.MapMonster"
$daBoss = New-Object System.Data.SqlClient.SqlDataAdapter $cmdBoss
$bossTable = New-Object System.Data.DataTable
[void]$daBoss.Fill($bossTable)
$bossNames = @{}
foreach ($row in $bossTable.Rows) {
    for ($i = 1; $i -le 3; $i++) {
        $bn = $row["BossMonster$i"]
        if ($bn -and $bn.ToString().Trim()) { $bossNames[$bn.ToString().Trim()] = $true }
    }
}

# -- Fetch all items --
Write-Host "Fetching items..." -ForegroundColor DarkGray
$cmdItem = $conn.CreateCommand()
$cmdItem.CommandTimeout = 0
$cmdItem.CommandText = @"
SELECT IDCode, [Name], CodeIMG1 AS Code, ReqLevel, ClassItem, DropFolder,
    ReqStrengh, ReqSpirit, ReqTalent, ReqAgility, ReqHealth,
    ModelPosition AS Tier
FROM dbo.ItemList
ORDER BY ReqLevel, CodeIMG1
"@
$daItem = New-Object System.Data.SqlClient.SqlDataAdapter $cmdItem
$items = New-Object System.Data.DataTable
[void]$daItem.Fill($items)

# Build item lookup: item code (e.g. "wa105") -> item data
$itemLookup = @{}
foreach ($row in $items.Rows) {
    $code = $row['Code'].ToString().Trim()
    if ($code) {
        $itemLookup[$code] = @{
            Name = $row['Name']; IDCode = $row['IDCode']; Level = [int]$row['ReqLevel']
            ClassItem = $row['ClassItem']; DropFolder = $row['DropFolder']; Tier = $row['Tier']
            ReqStr = $row['ReqStrengh']; ReqSpi = $row['ReqSpirit']; ReqTal = $row['ReqTalent']
            ReqAgi = $row['ReqAgility']; ReqHea = $row['ReqHealth']
        }
    }
}

# -- Fetch all drop entries --
Write-Host "Fetching drop data..." -ForegroundColor DarkGray
$cmdDrop = $conn.CreateCommand()
$cmdDrop.CommandTimeout = 0
$cmdDrop.CommandText = "SELECT DropID, Items, Chance FROM dbo.DropItem WHERE Items NOT IN ('Gold','Air') AND Items IS NOT NULL AND LTRIM(RTRIM(Items)) <> '' ORDER BY DropID, Chance DESC"
$daDrop = New-Object System.Data.SqlClient.SqlDataAdapter $cmdDrop
$drops = New-Object System.Data.DataTable
[void]$daDrop.Fill($drops)

$conn.Close()

# Build item -> monsters mapping
# itemCode -> list of {MonsterName, MonsterLevel, Chance}
Write-Host "Building item-drop lookup..." -ForegroundColor DarkGray
$itemDrops = @{}

foreach ($dropRow in $drops.Rows) {
    $dropId = [int]$dropRow['DropID']
    $monInfo = $monLookup[$dropId]
    if (-not $monInfo) { continue }

    $itemsStr = $dropRow['Items'].ToString().Trim()
    $chance = $dropRow['Chance']
    $codes = $itemsStr -split '\s+'

    foreach ($code in $codes) {
        $codeClean = $code.Trim().ToLower()
        if (-not $codeClean) { continue }
        if (-not $itemDrops[$codeClean]) { $itemDrops[$codeClean] = @() }
        $itemDrops[$codeClean] += [PSCustomObject]@{
            MonsterName = $monInfo.Name; MonsterLevel = $monInfo.Level; Chance = $chance
            IsBoss = $bossNames.ContainsKey($monInfo.Name)
        }
    }
}

# Categorize items
# Categorize items by code prefix
function Get-ItemCategory($code) {
    $c = $code.ToLower()

    # Prefix-based categories
    if ($c.StartsWith('pl')) { return 'Potions (HP)' }
    if ($c.StartsWith('pm')) { return 'Potions (MP)' }
    if ($c.StartsWith('ps')) { return 'Potions (SP)' }
    if ($c.StartsWith('os')) { return 'Sheltoms' }
    if ($c.StartsWith('ec')) { return 'Ether Cores' }
    if ($c.StartsWith('fo')) { return 'Force Orbs' }
    if ($c.StartsWith('qt')) { return 'Quest Items' }
    if ($c.StartsWith('gp')) { return 'Event Crystals' }
    if ($c.StartsWith('gg')) { return 'EXP/Gold Scrolls' }
    if ($c.StartsWith('bi')) { return 'Premium Items' }
    if ($c.StartsWith('sd')) { return 'Bellatra Items' }
    if ($c.StartsWith('qw')) { return 'Wings' }
    if ($c.StartsWith('se')) { return 'Respec Items' }
    if ($c.StartsWith('pr')) { return 'Crafting Items' }

    # Equipment by code prefix
    if ($c.StartsWith('wa')) { return 'Weapons (Axe)' }
    if ($c.StartsWith('wc')) { return 'Weapons (Claw)' }
    if ($c.StartsWith('wh')) { return 'Weapons (Hammer)' }
    if ($c.StartsWith('wm')) { return 'Weapons (Wand)' }
    if ($c.StartsWith('wp')) { return 'Weapons (Scythe)' }
    if ($c.StartsWith('ws1')) { return 'Weapons (Bow)' }
    if ($c.StartsWith('ws2')) { return 'Weapons (Sword)' }
    if ($c.StartsWith('wt')) { return 'Weapons (Javelin)' }
    if ($c.StartsWith('wn')) { return 'Weapons (Phantom)' }
    if ($c.StartsWith('wd')) { return 'Weapons (Dagger)' }

    if ($c.StartsWith('da1')) { return 'Armor' }
    if ($c.StartsWith('da2')) { return 'Robes' }
    if ($c.StartsWith('db')) { return 'Boots' }
    if ($c.StartsWith('dg')) { return 'Gauntlets' }
    if ($c.StartsWith('ds')) { return 'Shields' }
    if ($c.StartsWith('dr')) { return 'Robes' }
    if ($c.StartsWith('om')) { return 'Orbs' }
    if ($c.StartsWith('or')) { return 'Rings' }
    if ($c.StartsWith('oa1')) { return 'Amulets' }
    if ($c.StartsWith('oa2')) { return 'Bracelets' }

    return 'Other'
}

function Get-ReqString($itemInfo) {
    if (-not $itemInfo) { return '-' }
    $parts = @()
    $s = $itemInfo.ReqStr; if ($s -and [int]$s -gt 0) { $parts += "STR:$s" }
    $s = $itemInfo.ReqSpi; if ($s -and [int]$s -gt 0) { $parts += "SPI:$s" }
    $s = $itemInfo.ReqTal; if ($s -and [int]$s -gt 0) { $parts += "TAL:$s" }
    $s = $itemInfo.ReqAgi; if ($s -and [int]$s -gt 0) { $parts += "AGI:$s" }
    $s = $itemInfo.ReqHea; if ($s -and [int]$s -gt 0) { $parts += "HEA:$s" }
    if ($parts.Count -eq 0) { return '-' }
    return $parts -join ' '
}

Write-Host "Categorizing and generating markdown..." -ForegroundColor Cyan

$sb = New-Object System.Text.StringBuilder

[void]$sb.AppendLine('# Item Drop Reference')
[void]$sb.AppendLine()
[void]$sb.AppendLine('> Auto-generated from `GameDB.dbo.DropItem`, `GameDB.dbo.ItemList`, and `GameDB.dbo.MonsterList`')
[void]$sb.AppendLine("> Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
[void]$sb.AppendLine('> **Drop weight** = raw `Chance` value from `DropItem` table (higher = more likely within that monsters drop pool).')
[void]$sb.AppendLine()

# Collect all items that have drops
$allDropItems = @()
foreach ($code in $itemDrops.Keys) {
    $info = $itemLookup[$code]
    $cat = Get-ItemCategory $code
    $lvl = if ($info) { $info.Level } else { 999 }
    $allDropItems += [PSCustomObject]@{
        Code = $code; Info = $info; Category = $cat; Level = $lvl
    }
}

# Group by category
$cats = $allDropItems | Group-Object Category | Sort-Object Name

# Category order for display
$catOrder = @(
    'Weapons (Axe)', 'Weapons (Claw)', 'Weapons (Hammer)', 'Weapons (Wand)',
    'Weapons (Scythe)', 'Weapons (Bow)', 'Weapons (Sword)', 'Weapons (Javelin)',
    'Weapons (Phantom)', 'Weapons (Dagger)',
    'Armor', 'Robes', 'Shields', 'Boots', 'Gauntlets',
    'Orbs', 'Rings', 'Amulets', 'Bracelets',
    'Sheltoms', 'Force Orbs', 'Ether Cores', 'Crafting Items',
    'Potions (HP)', 'Potions (MP)', 'Potions (SP)',
    'Quest Items', 'EXP/Gold Scrolls', 'Event Crystals', 'Bellatra Items',
    'Wings', 'Respec Items', 'Premium Items', 'Other'
)

foreach ($catName in $catOrder) {
    $cat = $cats | Where-Object { $_.Name -eq $catName }
    if (-not $cat -or $cat.Count -eq 0) { continue }
    $catItems = $cat.Group | Sort-Object Level, Code

    [void]$sb.AppendLine("## $catName")
    [void]$sb.AppendLine()
    [void]$sb.AppendLine("$($catItems.Count) items")
    [void]$sb.AppendLine()
    [void]$sb.AppendLine('| Item | Code | Lvl | Reqs | Dropped By (drop weight) |')
    [void]$sb.AppendLine('| --- | --- | --- | --- | --- |')

    foreach ($ci in $catItems) {
        $code = $ci.Code
        $info = $ci.Info
        $name = if ($info) { $info.Name } else { $code }
        $lvl = if ($info) { $info.Level } else { '?' }
        $reqs = Get-ReqString $info

        # Get monsters that drop this, sorted by drop chance descending
        $dropList = $itemDrops[$code]
        # Deduplicate by monster name, keeping the highest chance
        $bestDrop = @{}
        foreach ($d in $dropList) {
            $key = $d.MonsterName
            if (-not $bestDrop[$key] -or $d.Chance -gt $bestDrop[$key].Chance) {
                $bestDrop[$key] = $d
            }
        }
        $allDrops = @($bestDrop.Values)
        $nonBossDrops = @($allDrops | Where-Object { -not $_.IsBoss })

        # Only show bosses if there are no non-boss droppers (bosses are the only source)
        if ($nonBossDrops.Count -gt 0) {
            $displayDrops = $nonBossDrops
        } else {
            $displayDrops = $allDrops
        }

        $uniqueDrops = $displayDrops | Sort-Object { -$_.Chance }
        $monsterStrs = foreach ($d in $uniqueDrops) {
            "$($d.MonsterName) ($($d.Chance))"
        }
        $monstersStr = if ($monsterStrs.Count -gt 0) { ($monsterStrs -join ', ') } else { '-' }
        if ($monstersStr.Length -gt 400) { $monstersStr = $monstersStr.Substring(0, 397) + '...' }

        [void]$sb.AppendLine("| $name | $code | $lvl | $reqs | $monstersStr |")
    }
    [void]$sb.AppendLine()
}

# Stats
$totalItems = $allDropItems.Count
$totalCodes = $itemDrops.Keys.Count
$matchedCodes = ($allDropItems | Where-Object { $_.Info -ne $null }).Count

[void]$sb.AppendLine('---')
[void]$sb.AppendLine()
[void]$sb.AppendLine("> **$totalCodes** unique item codes in drop tables. **$matchedCodes** matched to ItemList. **$totalItems** items shown above.")
[void]$sb.AppendLine("> Items not in `ItemList` but in drop tables may use `ItemListOld` codes or be invalid entries.")

$sb.ToString() | Out-File -FilePath $outputPath -Encoding utf8

Write-Host "Done! $totalCodes codes, $matchedCodes matched, $($cats.Count) categories." -ForegroundColor Green
Write-Host "Output written to: $outputPath" -ForegroundColor Green
