$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

Write-Host "[build] Building Part 1 simulator..."
New-Item -ItemType Directory -Force -Path "$Root\part1\bin" | Out-Null

gcc -std=c11 -O2 -Wall -Wextra -I"$Root\part1\include" `
  "$Root\part1\src\main.c" `
  "$Root\part1\src\runtime.c" `
  "$Root\part1\src\scenario.c" `
  "$Root\part1\src\tasks.c" `
  -o "$Root\part1\bin\sat_sim.exe"

Write-Host "[build] Build complete: part1/bin/sat_sim.exe"
