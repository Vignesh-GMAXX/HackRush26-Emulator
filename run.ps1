param(
    [string]$Scenario
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($Scenario)) {
    $Scenario = "$Root\part1\scenarios\sample_scenario.json"
}

if (!(Test-Path "$Root\part1\bin\sat_sim.exe")) {
    Write-Host "[run] Binary missing, invoking build.ps1 first..."
    & "$Root\build.ps1"
}

& "$Root\part1\bin\sat_sim.exe" $Scenario
