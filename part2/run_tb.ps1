$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

iverilog -g2012 -o "$Root\tb\activation_accel_tb.out" `
  "$Root\rtl\activation_accel.v" `
  "$Root\tb\activation_accel_tb.v"

vvp "$Root\tb\activation_accel_tb.out"
