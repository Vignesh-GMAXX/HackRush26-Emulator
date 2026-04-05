$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

Write-Host "[build] Building Part 1 simulator..."
New-Item -ItemType Directory -Force -Path "$Root\simulator_core\bin" | Out-Null

gcc -std=c11 -O2 -Wall -Wextra -I"$Root\simulator_core\include" `
  "$Root\simulator_core\src\main.c" `
  "$Root\simulator_core\src\runtime.c" `
  "$Root\simulator_core\src\scenario.c" `
  "$Root\simulator_core\src\tasks.c" `
  -o "$Root\simulator_core\bin\sat_sim.exe"

Write-Host "[build] Build complete: simulator_core/bin/sat_sim.exe"
