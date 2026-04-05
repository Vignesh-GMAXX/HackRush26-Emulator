#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "runtime.h"
#include "scenario.h"
#include "tasks.h"

#define SIM_CYCLES_PER_SEC 100000

static void print_summary(const scenario_t *sc, const runtime_stats_t *rt) {
    int64_t utilization_x100 = 0;
    if ((rt->active_cycles + rt->sleep_cycles) > 0) {
        utilization_x100 =
            ((rt->active_cycles * 10000) + ((rt->active_cycles + rt->sleep_cycles) / 2)) /
            (rt->active_cycles + rt->sleep_cycles);
    }

    int64_t remaining_energy_x10000 = ((int64_t)sc->energy_budget_mj * 10000) - rt->energy_mj_x10000;
    int64_t abs_remaining_x10000 = (remaining_energy_x10000 < 0) ? -remaining_energy_x10000 : remaining_energy_x10000;

    printf("\n================== FINAL RESOURCE REPORT ==================\n");
    
    printf("\n[CPU-UTILIZATION]\n");
    printf("  cpu_utilization_pct=%lld.%02lld\n",
           (long long)(utilization_x100 / 100),
           (long long)(utilization_x100 % 100));
    printf("  active_cycles=%lld\n", (long long)rt->active_cycles);
    printf("  sleep_cycles=%lld\n", (long long)rt->sleep_cycles);
    printf("  radio_cycles=%lld\n", (long long)rt->radio_cycles);
    printf("  total_cycles=%lld\n", (long long)rt->total_cycles);
    
    printf("\n[PER-TASK-CYCLES]\n");
    printf("  task_orbit_cycles=%lld\n", (long long)rt->task_cycles[TASK_ORBIT]);
    printf("  task_risk_cycles=%lld\n", (long long)rt->task_cycles[TASK_RISK]);
    printf("  task_maneuver_cycles=%lld\n", (long long)rt->task_cycles[TASK_MANEUVER]);
    printf("  task_telemetry_cycles=%lld\n", (long long)rt->task_cycles[TASK_TELEMETRY]);
    printf("  task_tx_cycles=%lld\n", (long long)rt->task_cycles[TASK_TX]);
    
    printf("\n[ENERGY-BUDGET]\n");
        printf("  estimated_energy_mj=%lld.%02lld\n",
            (long long)(rt->energy_mj_x10000 / 10000),
            (long long)((rt->energy_mj_x10000 % 10000) / 100));
    printf("  budget_mj=%d\n", sc->energy_budget_mj);
        printf("  remaining_energy_mj=%s%lld.%02lld\n",
            (remaining_energy_x10000 < 0) ? "-" : "",
            (long long)(abs_remaining_x10000 / 10000),
            (long long)((abs_remaining_x10000 % 10000) / 100));
        printf("  battery_status=%s\n", (remaining_energy_x10000 >= 0) ? "HEALTHY" : "EXCEEDED");
    
    printf("\n=========================================================\n");
}

int main(int argc, char **argv) {
    const char *scenario_path = (argc > 1) ? argv[1] : "simulator_core/scenarios/sample_scenario.json";

    scenario_t sc;
    int rc = load_scenario_file(scenario_path, &sc);
    if (rc != 0) {
        fprintf(stderr, "[error] failed to load scenario file: %s\n", scenario_path);
        return 1;
    }

    runtime_stats_t rt;
    runtime_init(&rt);

    mission_state_t ms;
    mission_init(&ms, &sc);

    printf("================== MISSION INITIALIZATION ==================\n");
    printf("[SCENARIO-PARAMETERS]\n");
    printf("  horizon_seconds=%d\n", sc.horizon_s);
    printf("  decision_window_seconds=%d\n", sc.decision_step_s);
    printf("  debris_objects_tracked=%d\n", sc.debris_count);
    printf("  energy_budget_mj=%d\n", sc.energy_budget_mj);
    printf("  satellite_r_m=%d\n", sc.sat_r_m);
    printf("  satellite_theta_mdeg=%d\n", sc.sat_theta_mdeg);
    printf("  satellite_omega_mdegps=%d\n", sc.sat_omega_mdegps);
    printf("=========================================================\n\n");

    for (int32_t now_s = 0; now_s <= sc.horizon_s; now_s++) {
        int64_t used_cycles = 0;

        if (now_s % 5 == 0) {
            int64_t before = rt.total_cycles;
            task_orbit_propagation(&ms, &sc, &rt, now_s);
            used_cycles += (rt.total_cycles - before);
        }

        if (now_s % 10 == 0) {
            int64_t before = rt.total_cycles;
            task_risk_eval(&ms, &sc, &rt, now_s);
            used_cycles += (rt.total_cycles - before);

            before = rt.total_cycles;
            task_maneuver(&ms, &sc, &rt, now_s);
            used_cycles += (rt.total_cycles - before);
        }

        if (now_s % 20 == 0) {
            int64_t before = rt.total_cycles;
            task_telemetry(&ms, &sc, &rt, now_s);
            used_cycles += (rt.total_cycles - before);

            before = rt.total_cycles;
            task_transmission_stage(&ms, &rt, now_s);
            used_cycles += (rt.total_cycles - before);
        }

        if (used_cycles < SIM_CYCLES_PER_SEC) {
            runtime_charge_sleep(&rt, SIM_CYCLES_PER_SEC - used_cycles);
        }
    }

    print_summary(&sc, &rt);
    return 0;
}
