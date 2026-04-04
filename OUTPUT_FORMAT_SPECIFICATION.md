# Satellite-Debris Simulation - Structured Output Format

## Overview
The simulation now produces strictly formatted logs with three main sections:
1. **Collision-Risk Reports** - Time-stamped debris risk classification
2. **Maneuver Recommendations** - HIGH-RISK event responses with Δv vector
3. **Resource Reports** - CPU, cycles, and energy tracking

---

## 1. Mission Initialization

```
================== MISSION INITIALIZATION ==================
[SCENARIO-PARAMETERS]
  horizon_seconds=120
  decision_window_seconds=10
  debris_objects_tracked=256
  energy_budget_mj=5000
  satellite_r_m=6800000
  satellite_theta_mdeg=0
  satellite_omega_mdegps=60
=========================================================
```

**Fields:**
- `horizon_seconds`: Simulation duration (seconds)
- `decision_window_seconds`: Task execution period
- `debris_objects_tracked`: Number of debris objects in scenario
- `energy_budget_mj`: Total available energy (millijoules)
- `satellite_r_m`: Satellite orbital radius (meters)
- `satellite_theta_mdeg`: Satellite angular position (millidegrees)
- `satellite_omega_mdegps`: Satellite angular velocity (millidegrees/second)

---

## 2. Collision-Risk Reports

### Timestamped Risk Classification

```
[COLLISION-RISK-REPORT] timestamp=0_s
  [risk-item] debris_id=100 class=WATCH proximity=243 size_cm=10 dr_m=195 dtheta_mdeg=9710
  [risk-item] debris_id=101 class=WATCH proximity=236 size_cm=11 dr_m=165 dtheta_mdeg=14215
  [risk-item] debris_id=102 class=HIGH-RISK proximity=85 size_cm=12 dr_m=15 dtheta_mdeg=18720
[COLLISION-RISK-SUMMARY] high_risk=1 watch=2 safe=253
```

**Risk Classifications:**
- **SAFE**: `proximity >= 100 * debris.size_cm` - No immediate threat
- **WATCH**: `10 * debris.size_cm <= proximity < 100 * debris.size_cm` - Monitor closely
- **HIGH-RISK**: `proximity < 10 * debris.size_cm` - Immediate threat, maneuver recommended

**Risk Item Fields:**
- `debris_id`: Unique debris identifier
- `class`: Risk level (SAFE/WATCH/HIGH-RISK)
- `proximity`: Computed proximity metric = dr + (dtheta / 200)
- `size_cm`: Debris size (centimeters)
- `dr_m`: Radial separation (meters) = |debris_r - satellite_r|
- `dtheta_mdeg`: Angular separation (millidegrees, normalized to ±180°)

**Summary Fields:**
- `high_risk`: Count of HIGH-RISK debris objects
- `watch`: Count of WATCH-level debris objects
- `safe`: Count of SAFE debris objects

---

## 3. Maneuver Recommendations

### HIGH-RISK Event Response

```
[MANEUVER-RECOMMENDATION] timestamp=30_s
  [maneuver-target] debris_id=104 risk_level=HIGH-RISK
  [maneuver-action] type=thruster_burn time_to_execute_s=2 delta_v_cms=-3 delta_r_m=-1
  [maneuver-status] executed=true
```

**Maneuver Fields:**
- `timestamp`: Time of maneuver decision (seconds)
- `debris_id`: Target debris object
- `risk_level`: Always HIGH-RISK for maneuver recommendations
- `type`: Maneuver type (thruster_burn)
- `time_to_execute_s`: Time to complete maneuver (seconds)
- `delta_v_cms`: Tangential burn command (centimeters/second)
  - Negative: Decrease angular velocity
  - Positive: Increase angular velocity
- `delta_r_m`: Change in orbital radius (meters)
  - Negative: Lower orbit
  - Positive: Raise orbit
- `executed`: Maneuver execution status (true/false)

---

## 4. Telemetry Reports

### Periodic Status Summary

```
[TELEMETRY-REPORT] timestamp=20_s
  [satellite-state] r_m=6800001 theta_mdeg=4500 omega_mdegps=54
  [high-risk-list] count=2 ids=104,219
  [planned-maneuver] target_id=104 delta_v_cms=-3 delta_r_m=-1
  [resource-usage] cpu_utilization_pct=8.96 total_cycles=100000 active_cycles=8960 sleep_cycles=91040 radio_cycles=1900
  [energy-status] consumed_mj=319.72 budget_mj=5000 remaining_mj=4680.28 battery_status=HEALTHY
  [risk-tracking] high_risk_so_far=5 high_risk_prev_20s=2
  [mission-status] debris_tracked=256 decision_window_s=20
```

**Satellite State:**
- `r_m`: Current orbital radius (meters)
- `theta_mdeg`: Current angular position (millidegrees)
- `omega_mdegps`: Current angular velocity (millidegrees/second)

**High-Risk List:**
- `count`: Number of HIGH-RISK objects currently detected
- `ids`: Comma-separated HIGH-RISK debris IDs (`none` when empty)

**Planned Maneuver Snapshot:**
- `target_id`: Last HIGH-RISK target selected by planner
- `delta_v_cms`: Planned tangential burn (cm/s)
- `delta_r_m`: Planned radial shift (m)

**Resource Usage Snapshot:**
- `cpu_utilization_pct`: Active cycle fraction percentage up to current timestamp
- `total_cycles`, `active_cycles`, `sleep_cycles`, `radio_cycles`: Cumulative runtime counters

**Energy Snapshot:**
- `consumed_mj`: Cumulative estimated energy consumed
- `budget_mj`: Scenario energy budget
- `remaining_mj`: Remaining budget (`budget_mj - consumed_mj`)
- `battery_status`: HEALTHY or EXCEEDED

**Risk Tracking (Per 20-second window):**
- `high_risk_so_far`: Cumulative HIGH-RISK detections across mission
- `high_risk_prev_20s`: HIGH-RISK detections in the latest 20s telemetry window

**Mission Status:**
- `debris_tracked`: Total debris objects in scenario
- `decision_window_s`: Telemetry transmission window (20s)

---

## 5. Transmission Logs

### Radio Communication Status

```
[TRANSMISSION-LOG] timestamp=20_s
  [transmission-packet] packet_number=2 type=weather_status status=sent radio_on=true
```

**Transmission Fields:**
- `timestamp`: Transmission time (seconds)
- `packet_number`: Sequential packet ID
- `type`: Packet type (weather_status)
- `status`: Transmission result (sent)
- `radio_on`: Radio active flag (true/false)

---

## 6. Final Resource Report

### Comprehensive System Performance Summary

```
================== FINAL RESOURCE REPORT ==================

[CPU-UTILIZATION]
  cpu_utilization_pct=8.96
  active_cycles=8960
  sleep_cycles=91040
  radio_cycles=1900
  total_cycles=100000

[PER-TASK-CYCLES]
  task_orbit_cycles=2000
  task_risk_cycles=4760
  task_maneuver_cycles=300
  task_telemetry_cycles=1100
  task_tx_cycles=800

[ENERGY-BUDGET]
  estimated_energy_mj=319.72
  budget_mj=5000
  remaining_energy_mj=4680.28
  battery_status=HEALTHY

=========================================================
```

**CPU Utilization Metrics:**
- `cpu_utilization_pct`: Percentage of time CPU actively executing tasks (vs. sleep)
- `active_cycles`: Total cycles spent in active computation
- `sleep_cycles`: Total cycles spent in power-saving mode
- `radio_cycles`: Total cycles spent with radio active
- `total_cycles`: Sum of active and sleep cycles (sim duration × cycles/sec)

**Per-Task Cycles Breakdown:**
- `task_orbit_cycles`: Orbital propagation computation
- `task_risk_cycles`: Risk evaluation and classification
- `task_maneuver_cycles`: Maneuver decision making
- `task_telemetry_cycles`: Telemetry packet preparation
- `task_tx_cycles`: Radio transmission

**Energy Budget:**
- `estimated_energy_mj`: Total energy consumed (millijoules)
- `budget_mj`: Available energy budget (millijoules)
- `remaining_energy_mj`: Energy remaining after simulation
- `battery_status`: HEALTHY (within budget) or EXCEEDED

---

## Output Generation Schedule

| Timestamp | Tasks Executed |
|-----------|----------------|
| Every 5s  | Orbit propagation |
| Every 10s | Risk evaluation + Maneuver decisions |
| Every 20s | Telemetry report + Transmission |
| Every 10s | Risk report (always) |
| End       | Final resource report |

---

## Example Scenario Output

See `results/edge_flat_max_debris.log` for a complete example with multiple risk reports, maneuver events, and resource tracking across a 120-second simulation with 256 debris objects.

---

## Compliance Checklist

✓ Collision-risk reports: Time-stamped list with SAFE/WATCH/HIGH-RISK classification  
✓ Maneuver recommendations: HIGH-RISK events with Δv and Δr vectors, execution time  
✓ Resource reports: CPU utilization, per-task cycles, energy usage, battery level  
✓ Structured format: Consistent bracketed sections, hierarchical indentation  
✓ All scenarios: Edge cases verified (zero horizon, zero debris, max debris, etc.)
