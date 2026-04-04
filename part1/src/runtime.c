#include "runtime.h"

#include <string.h>

static const double ACTIVE_MJ_PER_CYCLE = 0.020;
static const double SLEEP_MJ_PER_CYCLE = 0.0005;
static const double RADIO_MJ_PER_CYCLE = 0.050;

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

    rt->energy_mj += (double)cycles * ACTIVE_MJ_PER_CYCLE;
    if (radio_on) {
        rt->energy_mj += (double)cycles * RADIO_MJ_PER_CYCLE;
    }
}

void runtime_charge_sleep(runtime_stats_t *rt, int64_t cycles) {
    if (cycles <= 0) {
        return;
    }
    rt->sleep_cycles += cycles;
    rt->energy_mj += (double)cycles * SLEEP_MJ_PER_CYCLE;
}
