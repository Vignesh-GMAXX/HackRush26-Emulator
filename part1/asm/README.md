# MIPS32 Assembly Kernels

This folder contains MIPS32 assembly implementations of core runtime logic from the C simulator.

## File
- `runtime_kernels_mips32.s`

## Exported Symbols
- `mips_scheduler_flags(now_s)`
  - Returns bitmask for periodic task releases:
    - bit0: every 5s (orbit)
    - bit1: every 10s (risk + maneuver)
    - bit2: every 20s (telemetry + tx)
- `mips_abs_i32(x)`
- `mips_risk_classify(dr, dtheta)`
  - Mirrors C thresholds used in `task_risk_eval`:
    - HIGH-RISK if proximity < 60
    - WATCH if proximity < 220
    - SAFE otherwise
- `mips_should_maneuver(high_risk_count)`

## Notes
- Written using o32 calling convention and standard MIPS32 instructions.
- Intended for cross-compilation/emulation paths; not wired into the current x86-hosted build.
- This gives you a concrete assembly deliverable aligned to the current runtime behavior.
