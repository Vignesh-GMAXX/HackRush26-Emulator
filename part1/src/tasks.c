#include "tasks.h"
#include <stdio.h>

static int32_t abs_i32(int32_t x) {
    return (x < 0) ? -x : x;
}
static int32_t wrap_theta_mdeg(int32_t theta_mdeg) {
    while (theta_mdeg >= 360000) {
        theta_mdeg -= 360000;
    }
    while (theta_mdeg < 0) {
        theta_mdeg += 360000;
    }
    return theta_mdeg;
}
static int32_t signed_delta_theta_mdeg(int32_t debris_theta_mdeg, int32_t sat_theta_mdeg) {
    int32_t d = debris_theta_mdeg - sat_theta_mdeg;
    while (d > 180000) {
        d -= 360000;
    }
    while (d < -180000) {
        d += 360000;
    }
    return d;
}
static int32_t tangential_to_theta_delta_mdeg(int32_t vt_mps, int32_t radius_m, int32_t dt_s) {
    if (vt_mps == 0 || radius_m <= 0 || dt_s <= 0) {
        return 0;
    }

    /* 1 radian ~= 57296 millidegrees. Keep this as integer math to avoid FPU use. */
    const int64_t rad_to_mdeg = 57296LL;
    int64_t numerator = (int64_t)vt_mps * (int64_t)dt_s * rad_to_mdeg;

    if (numerator >= 0) {
        numerator += radius_m / 2;
    } else {
        numerator -= radius_m / 2;
    }

    return (int32_t)(numerator / radius_m);
}
static int32_t tangential_dv_to_omega_delta_mdegps(int32_t dv_cms, int32_t radius_m) {
    if (dv_cms == 0 || radius_m <= 0) {
        return 0;
    }

    /* Convert tangential cm/s burn into millidegrees/second using fixed-point math. */
    const int64_t rad_to_mdeg = 57296LL;
    int64_t numerator = (int64_t)dv_cms * rad_to_mdeg;
    int64_t denominator = (int64_t)radius_m * 100LL;

    if (numerator >= 0) {
        numerator += denominator / 2;
    } else {
        numerator -= denominator / 2;
    }

    return (int32_t)(numerator / denominator);
}
static int32_t predict_next_proximity(const mission_state_t *ms, const debris_t *target, int32_t cand_dv, int32_t cand_dr) {
    const int32_t step_s = 10;
    int32_t sat_r_next = ms->sat_r_m + cand_dr;
    int32_t sat_omega_next = ms->sat_omega_mdegps + tangential_dv_to_omega_delta_mdegps(cand_dv, ms->sat_r_m);
    int32_t sat_theta_next = ms->sat_theta_mdeg + sat_omega_next * step_s;
    sat_theta_next = wrap_theta_mdeg(sat_theta_next);

    int32_t debris_r_next = target->r_m + target->vr_mps * step_s;
    int32_t debris_radius_ref = target->r_m;
    if (debris_r_next > 0) {
        debris_radius_ref = (target->r_m + debris_r_next) / 2;
    }
    int32_t debris_theta_next = wrap_theta_mdeg(
        target->theta_mdeg + tangential_to_theta_delta_mdeg(target->vt_mps, debris_radius_ref, step_s));

    int32_t dr = abs_i32(debris_r_next - sat_r_next);
    int32_t dtheta = abs_i32(signed_delta_theta_mdeg(debris_theta_next, sat_theta_next));
    return dr + (dtheta / 200);
}

static const char *risk_level_to_str(risk_level_t level) {
    switch (level) {
    case RISK_SAFE:
        return "SAFE";
    case RISK_WATCH:
        return "WATCH";
    case RISK_HIGH:
        return "HIGH-RISK";
    default:
        return "UNKNOWN";
    }
}

void mission_init(mission_state_t *ms, const scenario_t *sc) {
    ms->sat_r_m = sc->sat_r_m;
    ms->sat_theta_mdeg = sc->sat_theta_mdeg;
    ms->sat_omega_mdegps = sc->sat_omega_mdegps;
    ms->high_risk_count = 0;
    ms->high_risk_total_so_far = 0;
    ms->high_risk_window_20s = 0;
    ms->last_high_risk_id = -1;
    ms->maneuver_pending = 0;
    ms->planned_dv_cms = 0;
    ms->planned_dr_m = 0;
    ms->tx_counter = 0;
}

void task_orbit_propagation(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s) {
    (void)now_s;

    ms->sat_theta_mdeg = wrap_theta_mdeg(ms->sat_theta_mdeg + ms->sat_omega_mdegps * 5);

    for (int i = 0; i < sc->debris_count; i++) {
        int32_t current_r = sc->debris[i].r_m;
        int32_t next_r = current_r + (sc->debris[i].vr_mps * 5);
        int32_t radius_ref = current_r;
        if (next_r > 0) {
            radius_ref = (current_r + next_r) / 2;
        }

        sc->debris[i].theta_mdeg = wrap_theta_mdeg(
            sc->debris[i].theta_mdeg + tangential_to_theta_delta_mdeg(sc->debris[i].vt_mps, radius_ref, 5));
        sc->debris[i].r_m = next_r;
    }

    runtime_charge_cycles(rt, TASK_ORBIT, 1800 + 25 * sc->debris_count, 0);
}

void task_risk_eval(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s) {
    ms->high_risk_count = 0;
    ms->last_high_risk_id = -1;

    printf("\n[COLLISION-RISK-REPORT] timestamp=%d_s\n", now_s);

    int high_count = 0, watch_count = 0, safe_count = 0;
    
    for (int i = 0; i < sc->debris_count; i++) {
        int32_t dr = abs_i32(sc->debris[i].r_m - ms->sat_r_m);
        int32_t dtheta = abs_i32(sc->debris[i].theta_mdeg - ms->sat_theta_mdeg);
        if (dtheta > 180000) {
            dtheta = 360000 - dtheta;
        }

        int32_t proximity = dr + (dtheta / 200);
        int32_t high_risk_threshold = 10 * sc->debris[i].size_cm;
        int32_t watch_threshold = 100 * sc->debris[i].size_cm;
        risk_level_t level = RISK_SAFE;
        if (proximity < high_risk_threshold) {
            level = RISK_HIGH;
            ms->high_risk_count++;
            ms->last_high_risk_id = sc->debris[i].id;
            high_count++;
        } else if (proximity < watch_threshold) {
            level = RISK_WATCH;
            watch_count++;
        } else {
            safe_count++;
        }

        printf("  [risk-item] debris_id=%d class=%s proximity=%d size_cm=%d dr_m=%d dtheta_mdeg=%d\n",
               sc->debris[i].id,
               risk_level_to_str(level),
               proximity,
               sc->debris[i].size_cm,
               dr,
               dtheta);
    }
    
    printf("[COLLISION-RISK-SUMMARY] high_risk=%d watch=%d safe=%d\n", high_count, watch_count, safe_count);

    ms->maneuver_pending = (ms->high_risk_count > 0) ? 1 : 0;
    ms->high_risk_total_so_far += ms->high_risk_count;
    ms->high_risk_window_20s += ms->high_risk_count;

    runtime_charge_cycles(rt, TASK_RISK, 4200 + 70 * sc->debris_count, 0);
}

void task_maneuver(mission_state_t *ms, scenario_t *sc, runtime_stats_t *rt, int32_t now_s) {
    if (!ms->maneuver_pending) {
        runtime_charge_cycles(rt, TASK_MANEUVER, 300, 0);
        return;
    }

    int target_idx = -1;
    for (int i = 0; i < sc->debris_count; i++) {
        if (sc->debris[i].id == ms->last_high_risk_id) {
            target_idx = i;
            break;
        }
    }

    int32_t best_dv = 0;
    int32_t best_dr = 0;
    if (target_idx >= 0) {
        int32_t dv_candidates[] = {-3, 3};
        int32_t dr_candidates[] = {-1, 1};
        int32_t best_predicted_prox = -1;

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                int32_t cand_dv = dv_candidates[i];
                int32_t cand_dr = dr_candidates[j];
                int32_t predicted_prox = predict_next_proximity(ms, &sc->debris[target_idx], cand_dv, cand_dr);

                if (predicted_prox > best_predicted_prox) {
                    best_predicted_prox = predicted_prox;
                    best_dv = cand_dv;
                    best_dr = cand_dr;
                }
            }
        }
    }

    ms->planned_dv_cms = best_dv;
    ms->planned_dr_m = best_dr;
    ms->sat_omega_mdegps += tangential_dv_to_omega_delta_mdegps(best_dv, ms->sat_r_m);
    ms->sat_r_m += best_dr;
    ms->maneuver_pending = 0;

    printf("\n[MANEUVER-RECOMMENDATION] timestamp=%d_s\n", now_s);
    printf("  [maneuver-target] debris_id=%d risk_level=HIGH-RISK\n", ms->last_high_risk_id);
    printf("  [maneuver-action] type=thruster_burn time_to_execute_s=2 delta_v_cms=%d delta_r_m=%d\n", best_dv, best_dr);
    printf("  [maneuver-status] executed=true\n");
    
    runtime_charge_cycles(rt, TASK_MANEUVER, 2200, 0);
}

void task_telemetry(mission_state_t *ms, const scenario_t *sc, runtime_stats_t *rt, int32_t now_s) {
    printf("\n[TELEMETRY-REPORT] timestamp=%d_s\n", now_s);
    printf("  [satellite-state] r_m=%d theta_mdeg=%d omega_mdegps=%d\n",
           ms->sat_r_m,
           ms->sat_theta_mdeg,
           ms->sat_omega_mdegps);
    printf("  [risk-tracking] high_risk_so_far=%d high_risk_prev_20s=%d\n",
           ms->high_risk_total_so_far,
           ms->high_risk_window_20s);
    printf("  [mission-status] debris_tracked=%d decision_window_s=20\n", sc->debris_count);

    ms->high_risk_window_20s = 0;

    runtime_charge_cycles(rt, TASK_TELEMETRY, 1100, 1);
}

void task_transmission_stage(mission_state_t *ms, runtime_stats_t *rt, int32_t now_s) {
    ms->tx_counter++;
    printf("\n[TRANSMISSION-LOG] timestamp=%d_s\n", now_s);
    printf("  [transmission-packet] packet_number=%d type=weather_status status=sent radio_on=true\n", ms->tx_counter);
    
    runtime_charge_cycles(rt, TASK_TX, 800, 1);
}
