#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "runtime.h"
#include "scenario.h"
#include "tasks.h"

#define SIM_CYCLES_PER_SEC 100000

static void print_summary(const scenario_t *sc, const runtime_stats_t *rt) {
    double utilization = 0.0;
    if ((rt->active_cycles + rt->sleep_cycles) > 0) {
        utilization = (100.0 * (double)rt->active_cycles) / (double)(rt->active_cycles + rt->sleep_cycles);
    }

    printf("\n=== FINAL SUMMARY ===\n");
    printf("total_cycles=%lld\n", (long long)rt->total_cycles);
    printf("task_cycles.orbit=%lld\n", (long long)rt->task_cycles[TASK_ORBIT]);
    printf("task_cycles.risk=%lld\n", (long long)rt->task_cycles[TASK_RISK]);
    printf("task_cycles.maneuver=%lld\n", (long long)rt->task_cycles[TASK_MANEUVER]);
    printf("task_cycles.telemetry=%lld\n", (long long)rt->task_cycles[TASK_TELEMETRY]);
    printf("task_cycles.tx=%lld\n", (long long)rt->task_cycles[TASK_TX]);
    printf("active_cycles=%lld sleep_cycles=%lld radio_cycles=%lld\n",
           (long long)rt->active_cycles,
           (long long)rt->sleep_cycles,
           (long long)rt->radio_cycles);
    printf("cpu_utilization_pct=%.2f\n", utilization);
    printf("energy_mj=%.2f budget_mj=%d\n", rt->energy_mj, sc->energy_budget_mj);
    printf("budget_status=%s\n", (rt->energy_mj <= sc->energy_budget_mj) ? "OK" : "EXCEEDED");
}

int main(int argc, char **argv) {
    const char *scenario_path = (argc > 1) ? argv[1] : "part1/scenarios/sample_scenario.json";

    scenario_t sc;
    int rc = load_scenario_file(scenario_path, &sc);
    if (rc != 0) {
        fprintf(stderr, "[warn] using default scenario (failed to parse file: %s)\n", scenario_path);
    }

    runtime_stats_t rt;
    runtime_init(&rt);

    mission_state_t ms;
    mission_init(&ms, &sc);

    printf("[init] horizon_s=%d decision_step_s=%d debris_count=%d\n",
           sc.horizon_s, sc.decision_step_s, sc.debris_count);

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
