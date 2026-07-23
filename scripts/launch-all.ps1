[CmdletBinding()]
param(
    [switch]$SkipClient
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot

$apps = [System.Collections.Generic.List[hashtable]]::new()
$apps.Add(@{
    Name = 'Login Server'
    Exe  = Join-Path $repoRoot 'Files\Server\login-server\Server.exe'
    Cwd  = Join-Path $repoRoot 'Files\Server\login-server'
})
$apps.Add(@{
    Name = 'Game Server'
    Exe  = Join-Path $repoRoot 'Files\Server\game-server\Server.exe'
    Cwd  = Join-Path $repoRoot 'Files\Server\game-server'
})

if (-not $SkipClient) {
    $apps.Add(@{
        Name = 'Game Client'
        Exe  = Join-Path $repoRoot 'Files\Game\Game.exe'
        Cwd  = Join-Path $repoRoot 'Files\Game'
    })
}

$processes = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()

function Stop-AllChildren {
    Write-Host "`nStopping all launched processes..." -ForegroundColor Yellow
    foreach ($p in $processes) {
        if (-not $p.HasExited) {
            try {
                $p.Kill($true)
                $p.WaitForExit(5000) | Out-Null
                Write-Host "  Stopped $($p.ProcessName) (PID $($p.Id))"
            }
            catch {
                Write-Host "  Could not kill PID $($p.Id): $_"
            }
        }
    }
}

$null = Register-EngineEvent -SourceIdentifier PowerShell.Exiting -Action { Stop-AllChildren }
try {
    [Console]::TreatControlCAsInput = $false
}
catch { }

foreach ($app in $apps) {
    if (-not (Test-Path $app.Exe)) {
        Write-Warning "Not found: $($app.Exe)"
        continue
    }

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $app.Exe
    $psi.WorkingDirectory = $app.Cwd
    $psi.UseShellExecute = $false

    $proc = [System.Diagnostics.Process]::Start($psi)
    $processes.Add($proc)
    Write-Host "Started $($app.Name) -- PID $($proc.Id)" -ForegroundColor Green
}

Write-Host "`nAll apps launched. Press Ctrl+C or Shift+F5 (Stop Debugging) to stop all.`n" -ForegroundColor Cyan

try {
    while ($processes.Count -gt 0) {
        $exited = $processes | Where-Object { $_.HasExited }
        foreach ($p in $exited) {
            Write-Host "$($p.ProcessName) (PID $($p.Id)) exited with code $($p.ExitCode)" -ForegroundColor DarkGray
            $processes.Remove($p) | Out-Null
        }
        if ($processes.Count -eq 0) { break }
        Start-Sleep -Milliseconds 500
    }
}
finally {
    Stop-AllChildren
}

Write-Host "All processes stopped."
