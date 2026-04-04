# Part 2 - ML Activation Accelerator

This folder contains the Verilog implementation for the integer activation accelerator track.

## Files
- `rtl/activation_accel.v` - accelerator RTL
- `tb/activation_accel_tb.v` - self-checking testbench

## Run

With Icarus Verilog:

```powershell
iverilog -g2012 -o .\part2\tb\activation_accel_tb.out .\part2\rtl\activation_accel.v .\part2\tb\activation_accel_tb.v
vvp .\part2\tb\activation_accel_tb.out
```
