# RISC-V Assembly Kernels

This folder contains architecture-specific assembly helpers for the simulator.

## Files
- `runtime_kernels_mips32.s` — legacy MIPS32 reference implementation
- `runtime_kernels_riscv32.s` — RISC-V RV32IM implementation aligned with the current simulator math

## Exported Symbols in the RISC-V File
- `rv_scheduler_flags(now_s)`
  - Returns bitmask for periodic task releases:
    - bit0: every 5s (orbit)
    - bit1: every 10s (risk + maneuver)
    - bit2: every 20s (telemetry + tx)
- `rv_abs_i32(x)`
- `rv_wrap_theta_mdeg(theta_mdeg)`
- `rv_signed_delta_theta_mdeg(debris_theta_mdeg, sat_theta_mdeg)`
- `rv_proximity_score(dr_m, dtheta_mdeg)`
- `rv_risk_classify(proximity, size_cm)`
  - Mirrors the current C thresholds used in `task_risk_eval`:
    - HIGH-RISK if proximity < 100 × size_cm
    - WATCH if proximity < 1000 × size_cm
    - SAFE otherwise
- `rv_should_maneuver(high_risk_count)`

## Notes
- Written for the RISC-V RV32IM calling convention.
- Intended for later emulator work and architecture translation; it is not yet wired into the current host build.
- The legacy MIPS32 file is kept only as a reference.

## Emulator

Run the helper assembly with the Python emulator from the `part1/` directory:

```powershell
python emulator\riscv_emulator.py asm\runtime_kernels_riscv32.s rv_risk_classify 120 3
```

Example outputs:
- `rv_scheduler_flags 10` -> `7`
- `rv_proximity_score 50 4000` -> `70`
- `rv_risk_classify 70 1` -> `2`

## Simulator UI

Launch the Tkinter simulator window from the `part1/` directory:

```powershell
python emulator\gui.py
```

The UI provides open/load/step/run/prev controls and shows the source, current instruction listing, and emulator state.