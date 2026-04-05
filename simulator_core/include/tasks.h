#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

#include "runtime.h"
#include "scenario.h"

typedef enum {
    RISK_SAFE = 0,
    RISK_WATCH = 1,
    RISK_HIGH = 2
} risk_level_t;

typedef struct {
    int32_t sat_r_m;
    int32_t sat_theta_mdeg;
    int32_t sat_omega_mdegps;
    int32_t high_risk_count;
    int32_t high_risk_ids[MAX_DEBRIS];
    int32_t high_risk_total_so_far;
    int32_t high_risk_window_20s;
    int32_t last_high_risk_id;
    int32_t maneuver_pending;
    int32_t planned_dv_cms;
    int32_t planned_dr_m;
    int32_t tx_counter;
} mission_state_t;

void mission_init(mission_state_t *ms, const scenario_t *sc);
void task_orbit_propagation(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s);
void task_risk_eval(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s);
void task_maneuver(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s);
void task_telemetry(mission_state_t *ms, const scenario_t *sc, runtime_stats_t *rt, int32_t now_s);
void task_transmission_stage(mission_state_t *ms, runtime_stats_t *rt, int32_t now_s);

#endif
