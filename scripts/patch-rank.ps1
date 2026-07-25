param($CharacterName, $Rank)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$chrName = "$CharacterName.chr"

$paths = @(
    (Join-Path $repoRoot ('Files\Server\login-server\Data\Character\' + $chrName)),
    (Join-Path $repoRoot ('Files\Server\game-server\Data\Character\'  + $chrName))
)

$found = $false
foreach ($p in $paths) {
    if (-not (Test-Path $p)) { continue }
    $found = $true
    $bytes = [IO.File]::ReadAllBytes($p)
    $old = $bytes[0x184]
    Write-Host "$(Split-Path $p -Leaf): rank=$old -> $Rank"
    if ($old -eq $Rank) { continue }
    $bytes[0x184] = $Rank
    [IO.File]::WriteAllBytes($p, $bytes)
}
if (-not $found) { throw "No .chr file found for '$CharacterName'." }
Write-Host 'Done. Restart the game server.'
