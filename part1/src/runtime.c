#include "runtime.h"

#include <string.h>

/* Energy scale: 1 unit = 1e-4 mJ. */
static const int64_t ACTIVE_MJ_X10000_PER_CYCLE = 200;
static const int64_t SLEEP_MJ_X10000_PER_CYCLE = 5;
static const int64_t RADIO_MJ_X10000_PER_CYCLE = 500;

void runtime_init(runtime_stats_t *rt) {
    memset(rt, 0, sizeof(*rt));
}

void runtime_charge_cycles(runtime_stats_t *rt, int task_id, int64_t cycles, int radio_on) {
    if (cycles <= 0) {
        return;
    }
    if (task_id >= 0 && task_id < TASK_COUNT) {
        rt->task_cycles[task_id] += cycles;
    }
    rt->total_cycles += cycles;
    rt->active_cycles += cycles;
    if (radio_on) {
        rt->radio_cycles += cycles;
    }

    rt->energy_mj_x10000 += cycles * ACTIVE_MJ_X10000_PER_CYCLE;
    if (radio_on) {
        rt->energy_mj_x10000 += cycles * RADIO_MJ_X10000_PER_CYCLE;
    }
}

void runtime_charge_sleep(runtime_stats_t *rt, int64_t cycles) {
    if (cycles <= 0) {
        return;
    }
    rt->sleep_cycles += cycles;
    rt->energy_mj_x10000 += cycles * SLEEP_MJ_X10000_PER_CYCLE;
}
