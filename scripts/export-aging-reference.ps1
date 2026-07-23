[CmdletBinding()]
param(
    [string]$SqlServer = '127.0.0.1,1433',
    [string]$SqlUser = 'sa',
    [string]$SqlPassword = '632514Go'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Data

$repoRoot = Split-Path -Parent $PSScriptRoot
$outputPath = Join-Path $repoRoot 'docs\reference\aging-reference.md'

$connStr = "Server=$SqlServer;User ID=$SqlUser;Password=$SqlPassword;Encrypt=False;TrustServerCertificate=True;Database=GameDB"
$conn = New-Object System.Data.SqlClient.SqlConnection $connStr

Write-Host "Connecting to database..." -ForegroundColor Cyan
$conn.Open()

$cmd1 = $conn.CreateCommand()
$cmd1.CommandTimeout = 0
$cmd1.CommandText = "SELECT CodeIMG1 AS ItemCode, [Name], ReqLevel AS ReqLvl FROM dbo.ItemList WHERE CodeIMG1 LIKE 'OS%' ORDER BY ReqLevel, CodeIMG1"
$da1 = New-Object System.Data.SqlClient.SqlDataAdapter $cmd1
$sheltoms = New-Object System.Data.DataTable
[void]$da1.Fill($sheltoms)

$cmd2 = $conn.CreateCommand()
$cmd2.CommandTimeout = 0
$cmd2.CommandText = "SELECT AgeNumber, FailChance, Plus2Chance, Minus2Chance, Minus1Chance, BrokenChance FROM dbo.AgeList ORDER BY AgeNumber"
$da2 = New-Object System.Data.SqlClient.SqlDataAdapter $cmd2
$ageList = New-Object System.Data.DataTable
[void]$da2.Fill($ageList)

$conn.Close()

# -- Hardcoded sheltom aging requirements from itemserver.cpp iaSheltomAgingList --
# Each row = aging level (1-20). Values = sheltom tier suffix (3 = os103 Fadeo, 4 = os104 Sparky, etc.)
# Each occurrence = 1 sheltom of that tier.
$sheltomReqTable = @(
    @(3,3,4,4,5),                                    # Age +1
    @(3,3,4,4,5,5),                                  # Age +2
    @(3,3,4,4,5,5,6),                                # Age +3
    @(3,3,4,4,5,5,6,6),                              # Age +4
    @(3,3,4,4,5,5,6,6,7),                            # Age +5
    @(3,3,4,4,5,5,6,6,7,7),                          # Age +6
    @(3,3,4,4,5,5,6,6,7,7,8),                        # Age +7
    @(3,3,4,4,5,5,6,6,7,7,8,8),                      # Age +8
    @(4,4,5,5,6,6,7,7,8,8,9),                        # Age +9
    @(4,4,5,5,6,6,7,7,8,8,9,9),                      # Age +10
    @(5,5,6,6,7,7,8,8,9,9,10),                       # Age +11
    @(5,5,6,6,7,7,8,8,9,9,10,10),                    # Age +12
    @(6,6,7,7,8,8,9,9,10,10,11),                     # Age +13
    @(6,6,7,7,8,8,9,9,10,10,11,11),                  # Age +14
    @(7,7,8,8,9,9,10,10,11,11,12),                   # Age +15
    @(7,7,8,8,9,9,10,10,11,11,12,12),                # Age +16
    @(8,8,9,9,10,10,11,11,12,12,13),                 # Age +17
    @(8,8,9,9,10,10,11,11,12,12,13,13),              # Age +18
    @(9,9,10,10,11,11,12,12,13,13,14),               # Age +19
    @(9,9,10,10,11,11,12,12,13,13,14,14)             # Age +20
)

# Tier suffix -> os code lookup.  The hardcoded table uses tier suffix values
# where 3=os103, 4=os104, ..., 10=os110, 14=os114.
# Since os codes are "os" + 2-digit tier + optional sub-tier, we extract
# the tier from the ReqLvl ordering rather than parsing the code string.
$sheltomByTier = @()
foreach ($row in $sheltoms.Rows) {
    $sheltomByTier += @{ Code = $row['ItemCode'].ToString(); Name = $row['Name']; ReqLvl = $row['ReqLvl'] }
}
# Build map: tier suffix (3..14) -> sheltom info
# The hardcoded table uses: 3=os103, 4=os104, 5=os105, 6=os106, 7=os107,
# 8=os108, 9=os109, 10=os110, 11=os111, 12=os112, 13=os113, 14=os114
$tierToSheltom = @{}
foreach ($st in $sheltomByTier) {
    $code = $st.Code
    # os101..os114 map to tiers 1..14, os121 is tier 21 (not used in aging)
    if ($code -match '^os(\d+)$') {
        $num = [int]$Matches[1]
        if ($num -le 114) {
            $suffix = $num - 100  # 103 -> 3, 110 -> 10, 114 -> 14
            $tierToSheltom[$suffix] = $st
        }
    }
}

Write-Host "Generating markdown..." -ForegroundColor Cyan

$sb = New-Object System.Text.StringBuilder

[void]$sb.AppendLine('# Aging & Sheltom Requirements Reference')
[void]$sb.AppendLine()
[void]$sb.AppendLine('> Auto-generated from `Server/server/itemserver.cpp` (`iaSheltomAgingList`) and `GameDB.dbo.AgeList`')
[void]$sb.AppendLine("> Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
[void]$sb.AppendLine()

# -- Section 1: Sheltom Items --
[void]$sb.AppendLine('## Sheltom Items')
[void]$sb.AppendLine()
[void]$sb.AppendLine('| Code | Name | ReqLvl |')
[void]$sb.AppendLine('| --- | --- | --- |')
foreach ($row in $sheltoms.Rows) {
    [void]$sb.AppendLine("| $($row['ItemCode']) | $($row['Name']) | $($row['ReqLvl']) |")
}
[void]$sb.AppendLine()

# -- Section 2: Aging Requirements Table --
[void]$sb.AppendLine('## Sheltom Requirements by Aging Level')
[void]$sb.AppendLine()
[void]$sb.AppendLine('Source: `iaSheltomAgingList[20][12]` in `Server/server/itemserver.cpp`.')
[void]$sb.AppendLine('Each value is the sheltom tier suffix (3=os103 Fadeo, 4=os104 Sparky, etc). Each occurrence = 1 sheltom.')
[void]$sb.AppendLine()
[void]$sb.AppendLine('| Age Lv | Sheltoms Required | Breakdown |')
[void]$sb.AppendLine('| --- | --- | --- |')

for ($ageIdx = 0; $ageIdx -lt $sheltomReqTable.Count; $ageIdx++) {
    $row = $sheltomReqTable[$ageIdx]
    $ageLv = $ageIdx + 1

    # Group by tier
    $counts = @{}
    foreach ($tier in $row) {
        if ($tier -eq 0) { continue }
        if (-not $counts[$tier]) { $counts[$tier] = 0 }
        $counts[$tier]++
    }

    $total = ($row | Where-Object { $_ -ne 0 }).Count
    $parts = @()
    foreach ($tier in ($counts.Keys | Sort-Object)) {
        $st = $tierToSheltom[$tier]
        if ($st) {
            $parts += "$($counts[$tier])x $($st.Name) ($($st.Code))"
        } else {
            $codeFallback = "os1$tier"
            if ($tier -ge 10) { $codeFallback = "os$tier" }
            $parts += "$($counts[$tier])x $codeFallback"
        }
    }
    $breakdown = $parts -join ', '

    [void]$sb.AppendLine("| $ageLv | $total | $breakdown |")
}
[void]$sb.AppendLine()

# -- Section 3: Aging Probabilities --
[void]$sb.AppendLine('## Aging Probabilities by Aging Level')
[void]$sb.AppendLine()
[void]$sb.AppendLine('Raw values from `GameDB.dbo.AgeList`. Used as dice-threshold values by the game engine.')
[void]$sb.AppendLine()
[void]$sb.AppendLine('| Age Lv | +2 | Fail | -1 | -2 | Break |')
[void]$sb.AppendLine('| --- | --- | --- | --- | --- | --- |')

foreach ($row in $ageList.Rows) {
    [void]$sb.AppendLine("| $($row['AgeNumber']) | $($row['Plus2Chance']) | $($row['FailChance']) | $($row['Minus1Chance']) | $($row['Minus2Chance']) | $($row['BrokenChance']) |")
}
[void]$sb.AppendLine()

# -- Section 4: Combined View --
[void]$sb.AppendLine('## Full Reference (Combined)')
[void]$sb.AppendLine()
[void]$sb.AppendLine('| Age Lv | Sheltoms | +2 | Fail | -1 | -2 | Break |')
[void]$sb.AppendLine('| --- | --- | --- | --- | --- | --- | --- |')

foreach ($row in $ageList.Rows) {
    $ageLv = [int]$row['AgeNumber']
    $ageIdx = $ageLv - 1
    $sheltomRow = $sheltomReqTable[$ageIdx]
    $totalSheltoms = ($sheltomRow | Where-Object { $_ -ne 0 }).Count
    [void]$sb.AppendLine("| $ageLv | $totalSheltoms | $($row['Plus2Chance']) | $($row['FailChance']) | $($row['Minus1Chance']) | $($row['Minus2Chance']) | $($row['BrokenChance']) |")
}
[void]$sb.AppendLine()

# -- Section 5: Notes --
[void]$sb.AppendLine('## Notes')
[void]$sb.AppendLine()
[void]$sb.AppendLine('- **Sheltom requirements** are per aging attempt and depend on the current aging level, not the item level.')
[void]$sb.AppendLine('- Aging levels 1-5 are **safe** (no downgrade or break).')
[void]$sb.AppendLine('- Aging level 20 is the maximum.')
[void]$sb.AppendLine('- The `ALWAYS_AGING_SUCCESS` server.ini cheat skips probability checks.')
[void]$sb.AppendLine('- Sheltoms are consumed regardless of success or failure.')
[void]$sb.AppendLine('- Source: `iaSheltomAgingList` in `Server/server/itemserver.cpp`, copied verbatim.')

$sb.ToString() | Out-File -FilePath $outputPath -Encoding utf8

Write-Host "Done! Output written to: $outputPath" -ForegroundColor Green
