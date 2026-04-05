# Satellite Debris Collision-Avoidance Simulator

A real-time C11 simulation system for satellite orbital collision avoidance, risk assessment, and autonomous maneuver planning. The simulator evaluates proximity to tracked debris objects, classifies threats, and recommends corrective maneuvers while monitoring CPU cycles and energy consumption.

**Part of**: HackRush 2026 - Computer Architecture Project (IITGN)

---

## Table of Contents

- [Overview](#overview)
- [System Requirements](#system-requirements)
- [Project Structure](#project-structure)
- [Installation & Setup](#installation--setup)
- [Building the Project](#building-the-project)
- [Running the Simulator](#running-the-simulator)
- [RISC-V Emulator and Assembly](#risc-v-emulator-and-assembly)
- [Configuration](#configuration)
- [Output Format](#output-format)
- [Key Features](#key-features)
- [Example Usage](#example-usage)
- [Troubleshooting](#troubleshooting)
- [Documentation](#documentation)

---

## Overview

The Satellite Debris Collision-Avoidance Simulator provides an accurate model of:

- **Orbital mechanics**: Circular orbit propagation with angular velocity tracking
- **Proximity assessment**: Size-scaled risk thresholds for collision threat classification
- **Autonomous maneuvers**: Dual-parameter optimization (tangential burn + radial offset)
- **Resource accounting**: CPU cycle budgeting and energy consumption tracking
- **Telemetry tracking**: Cumulative and windowed risk history reporting

The system uses **fixed-point arithmetic** to eliminate floating-point overhead in real-time operations and is designed for embedded satellite mission planning.

---

## System Requirements

### Hardware
- **Processor**: Modern x86-64 compatible CPU
- **Memory**: Minimum 512 MB RAM
- **Storage**: ~50 MB for project files and scenario data

### Software
- **OS**: Windows 10 or later (or Linux/macOS with appropriate tools)
- **Compiler**: MinGW32 (GCC) with C11 support
  - Download: [MinGW-w64](https://www.mingw-w64.org/)
  - Or via package manager (e.g., `choco install mingw` on Windows with Chocolatey)
- **Build Tool**: mingw32-make (included with MinGW)
- **Shell**: PowerShell (for Windows) or Bash (for Linux/macOS)

### Verify Installation
```powershell
gcc --version
mingw32-make --version
```

Both should display version information confirming installation.

---

## Project Structure

```
Part 1/
├── simulator_core/
│   ├── bin/                    # Compiled executable (generated)
│   │   └── sat_sim.exe
│   ├── build/                  # Object files (generated)
│   ├── include/
│   │   ├── scenario.h          # Scenario loader interface
│   │   ├── tasks.h             # Mission tasks and state
│   │   └── runtime.h           # Cycle and energy accounting
│   ├── src/
│   │   ├── main.c              # Main simulation loop
│   │   ├── scenario.c          # JSON scenario parser
│   │   ├── tasks.c             # Core mission logic
│   │   └── runtime.c           # Resource accounting
│   ├── scenarios/
│   │   ├── conic_scenario_dataset.json    # Test scenario (6 debris, 1800s)
│   │   └── [other scenarios]
│   ├── asm/                    # RISC-V assembly kernels and examples
│   │   ├── runtime_kernels_riscv32.s
│   │   └── sort_10_registers_riscv32.s
│   ├── emulator/               # Python emulator and GUI tools
│   │   ├── riscv_emulator.py
│   │   ├── emulator.py
│   │   └── gui.py
│   ├── Makefile                # Build configuration
│   └── README.md               # Simulator documentation
├── docs/
│   ├── output_variable_report.md   # Complete output documentation
│   └── [other documentation]
└── results/
    ├── conic_scenario_dataset.log  # Latest simulation output
    └── [archived logs]
```

---

## Installation & Setup

### 1. Clone or Extract Project

Clone from repository or extract the project archive to your desired location:

```powershell
# Option A: Clone from Git
git clone https://github.com/your-username/satellite-simulator.git
cd satellite-simulator

# Option B: Extract from ZIP
# Extract the archive, then navigate to the extracted folder
cd path/to/extracted/satellite-simulator
```

### 2. Verify MinGW Installation

Ensure you have MinGW32 (GCC) installed and accessible from command line:

```powershell
gcc --version
mingw32-make --version
```

Both commands should display version information.

**If not found:**
- Install MinGW from [mingw-w64.org](https://www.mingw-w64.org/)
- On Windows: Add MinGW `bin` directory to your system PATH
  - `Control Panel → System → Environment Variables → Path`
  - Add entry like `C:\mingw64\bin` (adjust path to your installation)
  - Restart any open PowerShell windows for changes to take effect

### 3. Verify Project Structure

From the project root directory, verify the structure:

```powershell
# Navigate to the simulator directory
cd simulator_core

# List key directories
ls src/          # Should have .c source files
ls include/      # Should have .h header files
ls scenarios/    # Should have .json test scenarios
```

### 4. Create Results Directory

Create an output directory for logs:

```powershell
# From project root (same level as simulator_core/ folder)
if (!(Test-Path "results")) { mkdir "results" }
```

---

## Building the Project

### Clean and Build
```powershell
cd simulator_core
mingw32-make clean
mingw32-make
```

### Verify Build Success
```powershell
ls -la bin/sat_sim.exe
```

The executable should be ~200-400 KB depending on compiler flags.

### Makefile Targets

| Target | Purpose |
|--------|---------|
| `make` or `make all` | Compile project to `bin/sat_sim.exe` |
| `make clean` | Remove compiled objects and executable |
| `make run` | Build and run simulator with conic scenario (output to console) |
| `make run-log` | Build and run simulator (output to `../results/conic_scenario_dataset.log`) |

---

## Running the Simulator

### Quick Start: Run Default Test Scenario
```powershell
cd simulator_core
mingw32-make run-log
```

Output will be written to `results/conic_scenario_dataset.log`.

### View Output
```powershell
# View entire log
cat ../results/conic_scenario_dataset.log

# View final resource report
Select-String -Path "../results/conic_scenario_dataset.log" -Pattern "FINAL RESOURCE REPORT" -Context 15

# View all HIGH-RISK events
Select-String -Path "../results/conic_scenario_dataset.log" -Pattern "HIGH-RISK"
```

### Manual Execution
```powershell
cd simulator_core
bin\sat_sim.exe
```

---

## RISC-V Emulator and Assembly

The project includes a Python RISC-V emulator and assembly examples for step-by-step inspection.

### Run Assembly in CLI
```powershell
cd simulator_core
python emulator\riscv_emulator.py asm\sort_10_registers_riscv32.s sort_10_registers
```

Expected behavior:
- `sort_10_registers` sorts 10 constants in ascending order.
- Sorted values are stored in `s0-s9` and mirrored to `a0-a7`, `t0`, `t1`.

### Run Assembly in GUI
```powershell
cd simulator_core
python emulator\gui.py
```

In the GUI:
1. Open `asm/sort_10_registers_riscv32.s`.
2. Set Function to `sort_10_registers` (or click Load and let GUI auto-select).
3. Click Run.

The GUI shows:
- Assembly source
- Instruction listing
- Register/memory state
- Performance and power summary

---

## Configuration

### Scenario Format

Scenarios are JSON files placed in `scenarios/` directory. Example structure:

```json
{
  "simulation_parameters": {
    "horizon_s": 1800,
    "decision_step_s": 20,
    "energy_budget_mj": 150000
  },
  "satellite_initial_state": {
    "r_m": 6771000,
    "theta_deg": 0.0,
    "omega_rad_s": 0.001093
  },
  "debris_catalog": [
    {
      "id": 1,
      "r": 6772000,
      "theta": 10.5,
      "vr": -50,
      "vt": 100,
      "size": 0.05
    }
  ]
}
```

**Key Parameters**:
- `horizon_s`: Total mission duration (seconds)
- `decision_step_s`: Decision cadence (typically 20s)
- `energy_budget_mj`: Total allowed energy budget (millijoules)
- `r_m`: Satellite orbital radius (meters)
- `theta_deg`: Initial angular position (degrees)
- `omega_rad_s`: Angular velocity (radians/second)
- `debris_catalog`: Array of debris objects with `id`, `r`, `theta`, `vr`, `vt`, `size`

### Simulator Constants

Edit in `include/tasks.h` and `src/runtime.c`:

```c
// Cycle accounting
#define SIM_CYCLES_PER_SEC          100000

// Energy costs per cycle (fixed-point internal units)
#define ENERGY_ACTIVE_WORK_UNITS    200      // 0.020 mJ/cycle
#define ENERGY_SLEEP_UNITS          5        // 0.0005 mJ/cycle
#define ENERGY_RADIO_UNITS          500      // 0.050 mJ/cycle

// Risk thresholds (multipliers on debris size_cm)
#define HIGH_RISK_THRESHOLD_MULT    100      // proximity < 100 * size_cm
#define WATCH_THRESHOLD_MULT        1000     // proximity < 1000 * size_cm

// Task scheduling (seconds)
#define ORBIT_TASK_INTERVAL         5        // Every 5 seconds
#define RISK_TASK_INTERVAL          10       // Every 10 seconds
#define MANEUVER_TASK_INTERVAL      10       // Every 10 seconds
#define TELEMETRY_TASK_INTERVAL     20       // Every 20 seconds
#define TX_TASK_INTERVAL            20       // Every 20 seconds
```

After modification, rebuild:
```powershell
mingw32-make clean
mingw32-make
```

---

## Output Format

The simulator produces six major output blocks per run:

### 1. MISSION INITIALIZATION
Scenario parameters and initial satellite state.

```
[SCENARIO-PARAMETERS]
  horizon_seconds=1800
  decision_window_seconds=20
  debris_objects_tracked=6
  energy_budget_mj=150000
  satellite_r_m=6771000
  satellite_theta_mdeg=0
  satellite_omega_mdegps=62
```

### 2. COLLISION-RISK-REPORT
Printed every 10 seconds: debris classification and proximity scores.

```
[COLLISION-RISK-REPORT] timestamp=150_s
  [risk-item] debris_id=3 class=HIGH-RISK proximity=1275 size_cm=135 dr_m=1058 dtheta_mdeg=43498
[COLLISION-RISK-SUMMARY] high_risk=1 watch=0 safe=5
```

### 3. MANEUVER-RECOMMENDATION
Printed when HIGH-RISK debris detected: planned corrective action.

```
[MANEUVER-RECOMMENDATION] timestamp=150_s
  [maneuver-target] debris_id=3 risk_level=HIGH-RISK
  [maneuver-action] type=thruster_burn time_to_execute_s=2 delta_v_cms=-3 delta_r_m=1
  [maneuver-status] executed=true
```

### 4. TELEMETRY-REPORT
Printed every 20 seconds: satellite state, resource usage, risk history.

```
[TELEMETRY-REPORT] timestamp=160_s
  [satellite-state] r_m=6771001 theta_mdeg=10695 omega_mdegps=62
  [high-risk-list] count=1 ids=3
  [planned-maneuver] target_id=3 delta_v_cms=-3 delta_r_m=1
  [resource-usage] cpu_utilization_pct=1.03 total_cycles=180100000 active_cycles=1782570 sleep_cycles=178317430 radio_cycles=172900
  [energy-status] consumed_mj=134.86 budget_mj=150000 remaining_mj=14865.14 battery_status=HEALTHY
  [risk-tracking] high_risk_so_far=1 high_risk_prev_20s=1
  [mission-status] debris_tracked=6 decision_window_s=20
```

### 5. TRANSMISSION-LOG
Printed every 20 seconds: radio transmission events.

```
[TRANSMISSION-LOG] timestamp=160_s
  [transmission-packet] packet_number=9 type=weather_status status=sent radio_on=true
```

### 6. FINAL RESOURCE REPORT
Printed at mission end: cumulative resource usage and feasibility.

```
[CPU-UTILIZATION]
  cpu_utilization_pct=1.03
  active_cycles=1782570
  sleep_cycles=178317430
  radio_cycles=172900
  total_cycles=180100000

[PER-TASK-CYCLES]
  task_orbit_cycles=722000
  task_risk_cycles=861560
  task_maneuver_cycles=98000
  task_telemetry_cycles=100100
  task_tx_cycles=72800

[ENERGY-BUDGET]
  estimated_energy_mj=134.86
  budget_mj=150000
  remaining_energy_mj=14865.14
  battery_status=HEALTHY
```

**For detailed explanation of every variable**, see [docs/output_variable_report.md](docs/output_variable_report.md).

---

## Key Features

### Risk Classification

| Category | Condition | Action |
|----------|-----------|--------|
| **HIGH-RISK** | proximity < 100 × size_cm | Immediate maneuver |
| **WATCH** | 100 × size_cm ≤ proximity < 1000 × size_cm | Monitor |
| **SAFE** | proximity ≥ 1000 × size_cm | No action |

Where: `proximity = Δr + (Δθ / 200)`

### Proximity Calculation Magic

```c
proximity = dr_m + (dtheta_mdeg / 200)
```

- **Δr** = |r_debris - r_satellite| (radial distance in meters)
- **Δθ** = shortest angular separation (millidegrees, wrapped to ±180°)
- The division by 200 balances units: ~1 mdeg ≈ 5 meters at typical LEO radius

### Maneuver Planning

When HIGH-RISK debris is detected, the simulator evaluates a **2×2 candidate space**:
- Tangential burn: `-3` or `+3` cm/s
- Radial offset: `-1` or `+1` meter

The best candidate is selected based on predicted **next-step proximity improvement** (10-second lookahead).

### Energy Model

| Component | Cost | Note |
|-----------|------|------|
| Active task work | 0.020 mJ/cycle | Risk eval, maneuver planning |
| Sleep/idle | 0.0005 mJ/cycle | Between task intervals |
| Radio transmission | 0.050 mJ/cycle (additional) | When radio enabled |

All calculations use **fixed-point arithmetic** (internally: `energy_mj_x10000`, displayed in mJ with 0.01 precision).

### CPU Utilization Tracking

```
CPU Utilization (%) = (active_cycles / (active_cycles + sleep_cycles)) × 100
```

---

## Example Usage

### Run and Display Full Output
```powershell
cd simulator_core
mingw32-make run
```

### Run and Save to Log
```powershell
mingw32-make run-log
cat ../results/conic_scenario_dataset.log
```

### Search for HIGH-RISK Events
```powershell
Select-String -Path "../results/conic_scenario_dataset.log" -Pattern "HIGH-RISK"
```

### Extract Final Resource Report
```powershell
Select-String -Path "../results/conic_scenario_dataset.log" -Pattern "FINAL RESOURCE REPORT" -Context 20
```

### Count Risk Events Over Time
```powershell
$log = cat ../results/conic_scenario_dataset.log
[regex]::Matches($log, "HIGH-RISK").Count
```

### Monitor Energy Consumption
```powershell
Select-String -Path "../results/conic_scenario_dataset.log" -Pattern "consumed_mj=|remaining"
```

### View CPU Utilization Trend
```powershell
Select-String -Path "../results/conic_scenario_dataset.log" -Pattern "cpu_utilization_pct="
```

---

## Troubleshooting

### Build Issues

**Error: `mingw32-make: command not found`**
- **Cause**: MinGW not installed or not in PATH
- **Solution**: Install MinGW from [mingw-w64.org](https://www.mingw-w64.org/) and add to PATH

**Error: `gcc: command not found`**
- **Cause**: GCC not in system PATH
- **Solution**: Verify MinGW installation location and add `bin` directory to PATH

**Error: `undefined reference to 'function_name'`**
- **Cause**: Missing object files or linker error
- **Solution**: Run `mingw32-make clean && mingw32-make`

**Error: `error: unknown type name 'int32_t'`**
- **Cause**: Missing `<stdint.h>` include or non-standard compiler
- **Solution**: Ensure compiler supports C11 standard

### Runtime Issues

**Error: `Scenario file not found`**
- **Cause**: JSON scenario file missing or path incorrect
- **Solution**: Verify `scenarios/conic_scenario_dataset.json` exists

**Error: `Invalid JSON in scenario file`**
- **Cause**: Malformed JSON syntax
- **Solution**: Validate scenario JSON using online JSON validators

**Error: `Energy exceeded at timestamp X_s`**
- **Cause**: Mission used more energy than allocated budget
- **Solution**: Increase `energy_budget_mj` in scenario or reduce task frequency

**No output generated**
- **Cause**: Redirection issues or scenario loading failure
- **Solution**: Try `mingw32-make run` (console output) instead of `mingw32-make run-log`

### Performance

**Slow execution**
- **Solution**: Close unnecessary applications; try fewer debris objects

**Simulator crashes mid-run**
- **Solution**: Check scenario JSON is valid; increase stack size if needed

---

## Documentation

### Extended Documentation
- **[Output Variable Report](docs/output_variable_report.md)**  
  Complete explanation of all simulator outputs, calculations, and data flow

### Source Code Documentation
Review inline comments in:
- `src/main.c` — mission loop orchestration
- `src/tasks.c` — risk evaluation and maneuver planning
- `src/runtime.c` — cycle and energy accounting
- `src/scenario.c` — JSON parsing

---

## Development Workflow

### Making Code Changes
1. Edit source files in `src/` or headers in `include/`
2. Rebuild: `mingw32-make clean && mingw32-make`
3. Test: `mingw32-make run-log`
4. Review output: `cat ../results/conic_scenario_dataset.log`

### Creating Custom Scenarios
1. Create JSON file in `scenarios/` following the format above
2. Update `src/main.c` to load your scenario (or modify scenario loader)
3. Rebuild and test

### Performance Analysis
Use the per-task cycle breakdown in FINAL RESOURCE REPORT:
- High `task_orbit_cycles` → optimize orbit propagation
- High `task_risk_cycles` → scales linearly with debris count
- High `task_maneuver_cycles` → maneuver search is expensive

---

## Quick Reference

### Build & Run
```powershell
mingw32-make              # Build
mingw32-make run          # Build & run (console)
mingw32-make run-log      # Build & run (to file)
mingw32-make gui          # Launch the Tkinter simulator UI
mingw32-make clean        # Clean build
```

### Risk Thresholds
```
HIGH-RISK:  proximity < 100 * size_cm
WATCH:      proximity < 1000 * size_cm
SAFE:       otherwise
```

### Units & Conversions
| Unit | Meaning | Conversion |
|------|---------|-----------|
| mdeg | millidegree | 1000 mdeg = 1° |
| mdegps | °/second | 1000 mdegps = 1°/s |
| energy_mj_x10000 | internal energy | 10000 units = 1 mJ |

### Energy Costs
| Task | Cost | Applies |
|------|------|---------|
| Active work | 0.020 mJ/cycle | During tasks |
| Sleep | 0.0005 mJ/cycle | Idle time |
| Radio | 0.050 mJ/cycle | During transmission |

---

## Support & Contributions

For issues or questions:
1. Check this README
2. Review [docs/output_variable_report.md](docs/output_variable_report.md)
3. Examine source code comments
4. Check simulator output logs in `results/`

---

## License & Attribution

**Project**: HackRush 2026 - Computer Architecture  
**Institution**: IITGN (Indian Institute of Technology Gandhinagar)  
**Purpose**: Educational and research

---

## Changelog

### Version 1.0 (April 2026)
- Initial release with full simulation capabilities
- JSON scenario parsing with nested debris catalog support
- Fixed-point arithmetic throughout
- Comprehensive cycle and energy accounting
- Risk classification and autonomous maneuver planning
- Telemetry windowing and aggregation

---

**Last Updated**: April 5, 2026
