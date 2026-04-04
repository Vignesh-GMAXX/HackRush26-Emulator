# Output Variable Report

This document explains every variable printed by the simulator, how it is calculated, and why it matters. It is written against the current implementation in `part1/src/main.c`, `part1/src/tasks.c`, and `part1/src/runtime.c`.

## 1. Overview of Output Blocks

The program prints six major blocks:

1. **MISSION INITIALIZATION**  
   Describes the scenario that was loaded and the starting satellite state.
2. **COLLISION-RISK-REPORT**  
   Classifies every debris object as `SAFE`, `WATCH`, or `HIGH-RISK`.
3. **MANEUVER-RECOMMENDATION**  
   Shows the maneuver chosen when a HIGH-RISK object is detected.
4. **TELEMETRY-REPORT**  
   Periodic snapshot of satellite state, risk history, and resources.
5. **TRANSMISSION-LOG**  
   Reports each radio packet sent.
6. **FINAL RESOURCE REPORT**  
   End-of-run summary of CPU and energy usage.

---

## 2. Units Used in the Program

The code uses a few fixed-point units to avoid floating-point overhead in the runtime path:

- **mdeg** = millidegree  
  $1\,\text{degree} = 1000\,\text{mdeg}$
- **mdegps** = millidegrees per second  
  $1000\,\text{mdegps} = 1\,\text{degree/s}$
- **energy_mj_x10000** = energy stored internally in units of $10^{-4}$ mJ  
  $10000\,\text{units} = 1\,\text{mJ}$

These units are then formatted back into human-readable output with two decimal places.

---

## 3. MISSION INITIALIZATION

Printed format:

```text
================== MISSION INITIALIZATION ==================
[SCENARIO-PARAMETERS]
  horizon_seconds=...
  decision_window_seconds=...
  debris_objects_tracked=...
  energy_budget_mj=...
  satellite_r_m=...
  satellite_theta_mdeg=...
  satellite_omega_mdegps=...
=========================================================
```

### 3.1 `horizon_seconds`
- **Meaning:** Total simulated mission duration in seconds.
- **Source:** Scenario field `horizon_s`.
- **Significance:** Controls how long the loop in `main.c` runs.

### 3.2 `decision_window_seconds`
- **Meaning:** Scenario-level decision cadence for risk and maneuver logic.
- **Source:** Scenario field `decision_step_s`.
- **Significance:** Used to describe how often the mission makes decisions.

### 3.3 `debris_objects_tracked`
- **Meaning:** Number of debris objects loaded into the scenario.
- **Source:** `sc.debris_count`.
- **Significance:** Drives the size of each risk-evaluation pass and the computational load.

### 3.4 `energy_budget_mj`
- **Meaning:** Total allowed mission energy budget in millijoules.
- **Source:** Scenario loader converts the JSON energy budget into mJ.
- **Calculation:**
  - The loader reads the scenario energy input and stores it as `energy_budget_mj`.
- **Significance:** Used later to determine whether the mission stayed within power limits.

### 3.5 `satellite_r_m`
- **Meaning:** Initial satellite orbital radius in meters.
- **Source:** Loaded scenario state.
- **Significance:** Baseline radius used in all proximity calculations.

### 3.6 `satellite_theta_mdeg`
- **Meaning:** Initial satellite angular position in millidegrees.
- **Source:** Loaded scenario state.
- **Significance:** Baseline angular location used when comparing satellite and debris position.

### 3.7 `satellite_omega_mdegps`
- **Meaning:** Initial satellite angular velocity in millidegrees per second.
- **Source:** Scenario angular velocity, converted to mdeg/s.
- **Calculation:**
  $$
  \omega_{\text{mdegps}} \approx \text{round}\left(\omega_{\text{rad/s}} \times \frac{180000}{\pi}\right)
  $$
- **Significance:** Used by orbit propagation and maneuver prediction.

---

## 4. COLLISION-RISK-REPORT

Printed for every risk-evaluation step.

Example format:

```text
[COLLISION-RISK-REPORT] timestamp=150_s
  [risk-item] debris_id=3 class=HIGH-RISK proximity=1275 size_cm=135 dr_m=1058 dtheta_mdeg=43498
[COLLISION-RISK-SUMMARY] high_risk=1 watch=0 safe=5
```

### 4.1 `timestamp`
- **Meaning:** Simulation time in seconds when the risk report was generated.
- **Significance:** Ties each classification to a specific mission instant.

### 4.2 `debris_id`
- **Meaning:** Unique debris identifier.
- **Source:** Loaded from the scenario file.
- **Significance:** Lets the same object be tracked across multiple timestamps.

### 4.3 `class`
- **Meaning:** Risk category assigned to the debris object.
- **Possible values:** `SAFE`, `WATCH`, `HIGH-RISK`.
- **Significance:** Determines whether a maneuver is needed.

### 4.4 `proximity`
- **Meaning:** Combined radial-angular closeness score.
- **Calculation:**
  $$
  \text{proximity} = \Delta r + \frac{\Delta \theta}{200}
  $$
  where:
  $$
  \Delta r = |r_{\text{debris}} - r_{\text{sat}}|
  $$
  and `\Delta \theta` is the shortest angular separation in millidegrees.
- **Significance:** This is the main scalar used for risk classification.

### 4.5 `size_cm`
- **Meaning:** Debris size in centimeters.
- **Source:** The scenario loader converts the JSON `size` field to centimeters.
- **Calculation:**
  $$
  \text{size\_cm} \approx \text{round}(\text{size}_{\text{m}} \times 100)
  $$
- **Significance:** Larger debris gets looser safety thresholds.

### 4.6 `dr_m`
- **Meaning:** Radial distance between debris and satellite in meters.
- **Calculation:**
  $$
  \Delta r = |r_{\text{debris}} - r_{\text{sat}}|
  $$
- **Significance:** Measures how far apart the two objects are in orbital radius.

### 4.7 `dtheta_mdeg`
- **Meaning:** Angular separation in millidegrees.
- **Calculation:**
  - Compute the raw difference in angle.
  - Wrap it to the shortest arc around the circle.
  - If the separation exceeds $180000$ mdeg, replace it with $360000 - \Delta\theta$.
- **Significance:** Prevents the system from treating near-opposite wrap-around angles as large when they are actually close across the $0/360^\circ$ boundary.

### 4.8 Risk thresholds
The current implementation uses size-scaled thresholds:

- **HIGH-RISK:**
  $$
  \text{proximity} < 100 \times \text{size\_cm}
  $$
- **WATCH:**
  $$
  100 \times \text{size\_cm} \le \text{proximity} < 1000 \times \text{size\_cm}
  $$
- **SAFE:**
  $$
  \text{proximity} \ge 1000 \times \text{size\_cm}
  $$

### 4.9 Summary counts
- **`high_risk`**: Number of debris objects classified as HIGH-RISK at that timestamp.
- **`watch`**: Number of debris objects classified as WATCH.
- **`safe`**: Number of debris objects classified as SAFE.

**Significance:** These counts summarize the danger level of the environment at that risk-evaluation step.

---

## 5. MANEUVER-RECOMMENDATION

Printed when at least one HIGH-RISK debris object is detected.

Example format:

```text
[MANEUVER-RECOMMENDATION] timestamp=150_s
  [maneuver-target] debris_id=3 risk_level=HIGH-RISK
  [maneuver-action] type=thruster_burn time_to_execute_s=2 delta_v_cms=-3 delta_r_m=1
  [maneuver-status] executed=true
```

### 5.1 `timestamp`
- **Meaning:** Time at which the maneuver decision is made.
- **Significance:** Allows comparison between risk detection and action timing.

### 5.2 `debris_id` in `maneuver-target`
- **Meaning:** The specific debris object that triggered the maneuver.
- **Source:** `last_high_risk_id`.
- **Significance:** Identifies the threat the maneuver is designed to mitigate.

### 5.3 `risk_level`
- **Meaning:** The risk level of the selected target.
- **Value:** Always `HIGH-RISK` for a maneuver recommendation.
- **Significance:** Confirms that a maneuver is only proposed for urgent threats.

### 5.4 `type`
- **Meaning:** Maneuver type.
- **Current value:** `thruster_burn`.
- **Significance:** Describes the control action used.

### 5.5 `time_to_execute_s`
- **Meaning:** Assumed time to complete the maneuver.
- **Current value:** `2` seconds.
- **Significance:** Fixed execution-delay model used by the report.

### 5.6 `delta_v_cms`
- **Meaning:** Tangential burn command in centimeters per second.
- **Calculation:**
  - The code tries candidate burns of `-3` and `+3` cm/s.
  - It selects the candidate that maximizes predicted next-step separation from the selected debris.
- **Significance:** Changes the satellite’s angular speed, which changes future orbital phase.

### 5.7 `delta_r_m`
- **Meaning:** Radial orbit adjustment in meters.
- **Calculation:**
  - The code tries candidate changes of `-1` and `+1` meter.
  - It selects the candidate that maximizes predicted next-step separation.
- **Significance:** Moves the satellite slightly inward or outward to improve clearance.

### 5.8 `executed`
- **Meaning:** Whether the maneuver recommendation was actually applied.
- **Value:** `true` in the current implementation when a target exists.
- **Significance:** Confirms the control decision was committed.

### 5.9 Maneuver prediction calculation
The maneuver planner evaluates candidate actions by predicting the next proximity.

The next-step satellite angle is estimated as:
$$
\theta_{\text{next}} = \text{wrap}(\theta_{\text{sat}} + \omega_{\text{next}} \times 10)
$$
where `10` is the maneuver prediction step in seconds.

The angular rate update from `delta_v_cms` is computed using a fixed-point approximation:
$$
\Delta\omega_{\text{mdegps}} \approx \text{round}\left(\frac{\Delta v_{\text{cms}} \times 57296}{\text{radius}_{\text{m}} \times 100}\right)
$$

Then the planner recomputes proximity for each candidate and chooses the best one.

**Significance:** This is how the simulator decides which maneuver is directionally better.

---

## 6. TELEMETRY-REPORT

Printed every telemetry cycle.

Example format:

```text
[TELEMETRY-REPORT] timestamp=160_s
  [satellite-state] r_m=6771001 theta_mdeg=10695 omega_mdegps=62
  [high-risk-list] count=1 ids=3
  [planned-maneuver] target_id=3 delta_v_cms=-3 delta_r_m=1
  [resource-usage] cpu_utilization_pct=1.03 total_cycles=180100000 active_cycles=1782570 sleep_cycles=178317430 radio_cycles=172900
  [energy-status] consumed_mj=134.86 budget_mj=150000 remaining_mj=14865.14 battery_status=HEALTHY
  [risk-tracking] high_risk_so_far=1 high_risk_prev_20s=1
  [mission-status] debris_tracked=6 decision_window_s=20
```

### 6.1 `timestamp`
- **Meaning:** Time when telemetry is reported.
- **Significance:** Aligns the telemetry snapshot with the mission timeline.

### 6.2 `satellite-state`

#### `r_m`
- **Meaning:** Current satellite radius in meters.
- **Significance:** Current orbital altitude state.

#### `theta_mdeg`
- **Meaning:** Current satellite angle in millidegrees.
- **Significance:** Current orbital phase.

#### `omega_mdegps`
- **Meaning:** Current angular velocity in millidegrees per second.
- **Significance:** Shows how fast the satellite is moving around its orbit.

These values come from `mission_state_t` after orbit propagation and maneuvers have been applied.

### 6.3 `high-risk-list`

#### `count`
- **Meaning:** Number of HIGH-RISK debris objects currently detected.
- **Source:** `high_risk_count`.
- **Significance:** Shows how many immediate threats exist right now.

#### `ids`
- **Meaning:** Comma-separated list of HIGH-RISK debris IDs.
- **Source:** `high_risk_ids[]`.
- **Significance:** Identifies exactly which objects require attention.
- **Special case:** If there are no high-risk objects, the program prints `none`.

### 6.4 `planned-maneuver`

#### `target_id`
- **Meaning:** Debris ID selected by the maneuver planner.
- **Source:** `last_high_risk_id`.
- **Significance:** Identifies the object used for maneuver planning.

#### `delta_v_cms`
- **Meaning:** The planned tangential burn.
- **Source:** `planned_dv_cms`.
- **Significance:** Expected action if a maneuver is executed.

#### `delta_r_m`
- **Meaning:** The planned radial offset.
- **Source:** `planned_dr_m`.
- **Significance:** Shows the radial component of the planned control action.

### 6.5 `resource-usage`

#### `cpu_utilization_pct`
- **Meaning:** Fraction of cycles spent doing active work rather than sleeping.
- **Calculation:**
  $$
  \text{cpu\_utilization\_pct} = \frac{\text{active\_cycles}}{\text{active\_cycles} + \text{sleep\_cycles}} \times 100
  $$
- **Implementation detail:** The code computes this using fixed-point integer math and prints two decimal places.
- **Significance:** Measures runtime efficiency.

#### `total_cycles`
- **Meaning:** Total cycles elapsed in the simulation accounting model.
- **Calculation:**
  $$
  \text{total\_cycles} = \text{active\_cycles} + \text{sleep\_cycles}
  $$
- **Significance:** Full cycle count across both work and idle time.

#### `active_cycles`
- **Meaning:** Cycles spent executing tasks.
- **Significance:** Compute workload.

#### `sleep_cycles`
- **Meaning:** Cycles spent in idle/sleep mode.
- **Significance:** Time not spent on active processing.

#### `radio_cycles`
- **Meaning:** Cycles spent with the radio enabled.
- **Significance:** Communication load and power impact.

### 6.6 `energy-status`

#### `consumed_mj`
- **Meaning:** Total estimated energy consumed so far.
- **Internal calculation:** Energy is accumulated in `energy_mj_x10000`.
- **Energy model per cycle:**
  - active work: `200` units = `0.0200 mJ/cycle`
  - sleep: `5` units = `0.0005 mJ/cycle`
  - radio on: `500` extra units = `0.0500 mJ/cycle`
- **Displayed value:** printed in mJ with two decimal places.

#### `budget_mj`
- **Meaning:** Scenario energy budget in millijoules.
- **Significance:** The mission must stay within this limit.

#### `remaining_mj`
- **Meaning:** Remaining budget after consumed energy is subtracted.
- **Calculation:**
  $$
  \text{remaining\_mj} = \text{budget\_mj} - \text{consumed\_mj}
  $$
- **Significance:** Shows how much power margin is left.

#### `battery_status`
- **Meaning:** Mission energy feasibility indicator.
- **Rule:**
  - `HEALTHY` if remaining energy is non-negative.
  - `EXCEEDED` if remaining energy is negative.
- **Significance:** Final pass/fail indicator for the power model.

### 6.7 `risk-tracking`

#### `high_risk_so_far`
- **Meaning:** Cumulative number of HIGH-RISK detections since mission start.
- **Calculation:**
  $$
  \text{high\_risk\_so\_far} = \sum \text{high\_risk\_count at each risk-eval step}
  $$
- **Significance:** Long-term risk history.

#### `high_risk_prev_20s`
- **Meaning:** Number of HIGH-RISK detections accumulated since the previous telemetry report.
- **Calculation:**
  - `task_risk_eval()` adds the current step’s HIGH-RISK count into a 20-second window accumulator.
  - `task_telemetry()` prints that accumulator.
  - `task_telemetry()` then resets the window accumulator to zero.
- **Significance:** Short-term risk burst indicator.

### 6.8 `mission-status`

#### `debris_tracked`
- **Meaning:** Total number of debris objects in the loaded scenario.
- **Significance:** Mission complexity and workload indicator.

#### `decision_window_s`
- **Meaning:** Telemetry window length used for the current report.
- **Current value:** `20` seconds.
- **Significance:** Helps interpret `high_risk_prev_20s`.

---

## 7. TRANSMISSION-LOG

Printed every time a packet is sent.

Example format:

```text
[TRANSMISSION-LOG] timestamp=160_s
  [transmission-packet] packet_number=9 type=weather_status status=sent radio_on=true
```

### 7.1 `timestamp`
- **Meaning:** Time when the transmission occurs.
- **Significance:** Aligns communication events with the simulation timeline.

### 7.2 `packet_number`
- **Meaning:** Sequential packet index.
- **Source:** `tx_counter` increments every transmission.
- **Significance:** Lets you track the order of packets.

### 7.3 `type`
- **Meaning:** Packet type.
- **Current value:** `weather_status`.
- **Significance:** Describes the payload class being sent.

### 7.4 `status`
- **Meaning:** Transmission outcome.
- **Current value:** `sent`.
- **Significance:** Confirms the packet was emitted.

### 7.5 `radio_on`
- **Meaning:** Whether the radio was enabled for this transmission.
- **Current value:** `true`.
- **Significance:** Contributes to `radio_cycles` and extra energy cost.

---

## 8. FINAL RESOURCE REPORT

This block summarizes the whole mission after the simulation ends.

Example format:

```text
================== FINAL RESOURCE REPORT ==================

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

=========================================================
```

### 8.1 CPU-utilization fields
The final report uses the same accounting model as telemetry, but over the entire run.

#### `cpu_utilization_pct`
- **Meaning:** Final percentage of cycles spent actively working.
- **Calculation:**
  $$
  \text{cpu\_utilization\_pct} = \frac{\text{active\_cycles}}{\text{active\_cycles} + \text{sleep\_cycles}} \times 100
  $$
- **Significance:** Overall mission efficiency.

#### `active_cycles`
- **Meaning:** Total cycles spent in task execution.
- **Significance:** Aggregate compute effort.

#### `sleep_cycles`
- **Meaning:** Total idle cycles.
- **Significance:** Aggregate inactive time.

#### `radio_cycles`
- **Meaning:** Total cycles with radio active.
- **Significance:** Communication overhead.

#### `total_cycles`
- **Meaning:** Total accounted cycles over the mission.
- **Calculation:**
  $$
  \text{total\_cycles} = \text{active\_cycles} + \text{sleep\_cycles}
  $$
- **Significance:** Full cycle budget across active and idle time.

### 8.2 Per-task cycle fields
These values come from the `task_cycles[]` array in `runtime_stats_t`.

- `task_orbit_cycles`: cycles spent propagating the orbit.
- `task_risk_cycles`: cycles spent classifying debris risk.
- `task_maneuver_cycles`: cycles spent computing and applying maneuvers.
- `task_telemetry_cycles`: cycles spent preparing telemetry.
- `task_tx_cycles`: cycles spent transmitting data.

**Significance:** This breakdown shows which tasks are the main CPU consumers.

### 8.3 Energy-budget fields

#### `estimated_energy_mj`
- **Meaning:** Total energy used during the mission.
- **Internal calculation:**
  $$
  \text{energy\_mj\_x10000} =
  \sum (\text{active cycles} \times 200)
  + \sum (\text{sleep cycles} \times 5)
  + \sum (\text{radio cycles} \times 500)
  $$
  and then displayed as millijoules.
- **Significance:** Total energy consumption.

#### `budget_mj`
- **Meaning:** Allowed mission energy budget.
- **Significance:** Hard limit from the scenario.

#### `remaining_energy_mj`
- **Meaning:** Energy left at mission end.
- **Calculation:**
  $$
  \text{remaining\_energy\_mj} = \text{budget\_mj} - \text{estimated\_energy\_mj}
  $$
- **Significance:** Energy margin.

#### `battery_status`
- **Meaning:** Whether the mission stayed within budget.
- **Rule:**
  - `HEALTHY` if remaining energy is at least zero.
  - `EXCEEDED` otherwise.
- **Significance:** Final feasibility verdict for the power model.

---

## 9. How the values are updated over time

The simulator runs in 1-second increments. In each second:

- **Every 5 s:** orbit propagation runs.
- **Every 10 s:** risk evaluation and maneuver planning run.
- **Every 20 s:** telemetry and transmission run.
- **Every second:** any unused cycles up to `SIM_CYCLES_PER_SEC` are charged as sleep.

This means the output variables are not independent; they are built from the same runtime accounting data:
- risk counters come from `task_risk_eval()`,
- maneuver outputs come from the latest risk target,
- telemetry snapshots read the current mission state,
- final resource totals come from all accumulated runtime counters.

---

## 10. Key relationships between variables

- `high_risk_count` drives `maneuver_pending`.
- `high_risk_ids[]` is populated only for HIGH-RISK debris.
- `planned_dv_cms` and `planned_dr_m` are chosen by search over candidate maneuvers.
- `active_cycles`, `sleep_cycles`, and `radio_cycles` feed into energy calculation.
- `high_risk_so_far` and `high_risk_prev_20s` summarize risk history at different time scales.
- `battery_status` depends entirely on `remaining_energy_mj`.

---

## 11. Short interpretation guide

If you read the output top to bottom, the meaning is:

- **Initialization:** what scenario is being run.
- **Risk report:** what debris is dangerous right now.
- **Maneuver:** what action was chosen to reduce risk.
- **Telemetry:** current state, recent risk history, and resource usage.
- **Transmission:** what was sent over the radio.
- **Final resource report:** whether the mission stayed within CPU and energy limits.

---

## 12. Notes

- `decision_window_seconds` in initialization is a scenario parameter.
- `decision_window_s` in telemetry is the 20-second telemetry aggregation window.
- `high_risk_prev_20s` is reset after each telemetry print.
- All energy values are internally accumulated in `energy_mj_x10000` and printed as millijoules with two decimal places.

This report matches the current implementation and the current log format.
