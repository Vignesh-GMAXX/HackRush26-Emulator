param(
    [string]$Scenario
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($Scenario)) {
    $Scenario = "$Root\simulator_core\scenarios\simple_nominal_scenario.json"
}

if (!(Test-Path "$Root\simulator_core\bin\sat_sim.exe")) {
    Write-Host "[run] Binary missing, invoking build.ps1 first..."
    & "$Root\build.ps1"
}

& "$Root\simulator_core\bin\sat_sim.exe" $Scenario
