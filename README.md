# HackRush 2026 - Computer Architecture Project Starter

This repository contains the Part 1 Computer Architecture project:

- Part 1: Satellite Based Low-Power Onboard Computer and OS runtime (C, MIPS-friendly design)

## Structure

- `part1/`: C simulation runtime, scheduler, power and cycle accounting
- `docs/`: documentation and presentation templates
- `build.sh`: build Part 1 simulator
- `run.sh`: run Part 1 simulator with a scenario

## Quick Start

```bash
./build.sh
./run.sh part1/scenarios/sample_scenario.json
```

Windows PowerShell:

```powershell
.\build.ps1
.\run.ps1 .\part1\scenarios\sample_scenario.json
```

## Expected Output

The simulator prints:

- time-stamped risk reports
- maneuver decisions
- per-task cycle counters
- active/sleep/radio accounting
- estimated energy usage

Use redirection for evaluation logs:

```bash
./run.sh part1/scenarios/sample_scenario.json > output_sample.log
```
