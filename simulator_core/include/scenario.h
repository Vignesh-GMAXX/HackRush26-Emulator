#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdint.h>

#define MAX_DEBRIS 256

typedef struct {
    int32_t id;
    int32_t r_m;
    int32_t theta_mdeg;
    int32_t vr_mps;
    int32_t vt_mps;
    int32_t size_cm;
} debris_t;

typedef struct {
    int32_t horizon_s;
    int32_t decision_step_s;
    int32_t energy_budget_mj;
    int32_t sat_r_m;
    int32_t sat_theta_mdeg;
    int32_t sat_omega_mdegps;
    int32_t debris_count;
    debris_t debris[MAX_DEBRIS];
} scenario_t;

int load_scenario_file(const char *path, scenario_t *sc);

#endif
