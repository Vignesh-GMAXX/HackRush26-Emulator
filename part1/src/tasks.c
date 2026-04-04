#include "tasks.h"

#include <stdio.h>

static int32_t abs_i32(int32_t x) {
    return (x < 0) ? -x : x;
}

void mission_init(mission_state_t *ms, const scenario_t *sc) {
    ms->sat_r_m = sc->sat_r_m;
    ms->sat_theta_mdeg = sc->sat_theta_mdeg;
    ms->sat_omega_mdegps = sc->sat_omega_mdegps;
    ms->high_risk_count = 0;
    ms->last_high_risk_id = -1;
    ms->maneuver_pending = 0;
    ms->planned_dv_cms = 0;
    ms->planned_dr_m = 0;
    ms->tx_counter = 0;
}

void task_orbit_propagation(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s) {
    (void)now_s;

    ms->sat_theta_mdeg += ms->sat_omega_mdegps * 5;
    while (ms->sat_theta_mdeg >= 360000) {
        ms->sat_theta_mdeg -= 360000;
    }

    for (int i = 0; i < sc->debris_count; i++) {
        sc->debris[i].theta_mdeg += (sc->debris[i].vt_mps * 5);
        if (sc->debris[i].theta_mdeg >= 360000) {
            sc->debris[i].theta_mdeg -= 360000;
        }
        sc->debris[i].r_m += (sc->debris[i].vr_mps * 5);
    }

    runtime_charge_cycles(rt, TASK_ORBIT, 1800 + 25 * sc->debris_count, 0);
}

void task_risk_eval(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s) {
    ms->high_risk_count = 0;
    ms->last_high_risk_id = -1;

    for (int i = 0; i < sc->debris_count; i++) {
        int32_t dr = abs_i32(sc->debris[i].r_m - ms->sat_r_m);
        int32_t dtheta = abs_i32(sc->debris[i].theta_mdeg - ms->sat_theta_mdeg);
        if (dtheta > 180000) {
            dtheta = 360000 - dtheta;
        }

        int32_t proximity = dr + (dtheta / 200);
        risk_level_t level = RISK_SAFE;
        if (proximity < 60) {
            level = RISK_HIGH;
            ms->high_risk_count++;
            ms->last_high_risk_id = sc->debris[i].id;
        } else if (proximity < 220) {
            level = RISK_WATCH;
        }

        if (level == RISK_HIGH) {
            printf("[risk] t=%d id=%d class=HIGH-RISK prox=%d\n", now_s, sc->debris[i].id, proximity);
        }
    }

    ms->maneuver_pending = (ms->high_risk_count > 0) ? 1 : 0;
    runtime_charge_cycles(rt, TASK_RISK, 4200 + 70 * sc->debris_count, 0);
}

void task_maneuver(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s) {
    (void)sc;

    if (!ms->maneuver_pending) {
        runtime_charge_cycles(rt, TASK_MANEUVER, 300, 0);
        return;
    }

    int32_t candidate_dv[] = {0, 3, -3};
    int32_t candidate_dr[] = {0, 1, -1};

    int32_t best_score = -1;
    int32_t best_dv = 0;
    int32_t best_dr = 0;

    for (int i = 0; i < 3; i++) {
        int32_t score = (candidate_dv[i] >= 0 ? candidate_dv[i] : -candidate_dv[i]) * 6
            + (candidate_dr[i] >= 0 ? candidate_dr[i] : -candidate_dr[i]) * 12
            + ms->high_risk_count * 10;

        if (score > best_score) {
            best_score = score;
            best_dv = candidate_dv[i];
            best_dr = candidate_dr[i];
        }
    }

    ms->planned_dv_cms = best_dv;
    ms->planned_dr_m = best_dr;
    ms->sat_r_m += best_dr;
    ms->sat_omega_mdegps += best_dv;
    ms->maneuver_pending = 0;

    printf("[maneuver] t=%d target_id=%d dv_cms=%d dr_m=%d\n", now_s, ms->last_high_risk_id, best_dv, best_dr);
    runtime_charge_cycles(rt, TASK_MANEUVER, 2200, 0);
}

void task_telemetry(mission_state_t *ms, const scenario_t *sc, runtime_stats_t *rt, int32_t now_s) {
    printf("[telemetry] t=%d sat_r_m=%d sat_theta_mdeg=%d high_risk=%d debris=%d\n",
           now_s, ms->sat_r_m, ms->sat_theta_mdeg, ms->high_risk_count, sc->debris_count);

    runtime_charge_cycles(rt, TASK_TELEMETRY, 1100, 1);
}

void task_transmission_stage(mission_state_t *ms, runtime_stats_t *rt, int32_t now_s) {
    ms->tx_counter++;
    printf("[tx] t=%d weather_pkt=%d status=sent\n", now_s, ms->tx_counter);
    runtime_charge_cycles(rt, TASK_TX, 800, 1);
}
