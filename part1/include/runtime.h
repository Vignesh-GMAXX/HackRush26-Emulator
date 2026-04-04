#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>

enum {
    TASK_ORBIT = 0,
    TASK_RISK = 1,
    TASK_MANEUVER = 2,
    TASK_TELEMETRY = 3,
    TASK_TX = 4,
    TASK_IDLE = 5,
    TASK_COUNT = 6
};

typedef struct {
    int64_t total_cycles;
    int64_t task_cycles[TASK_COUNT];
    int64_t active_cycles;
    int64_t sleep_cycles;
    int64_t radio_cycles;
    int64_t energy_mj_x10000;
} runtime_stats_t;

void runtime_init(runtime_stats_t *rt);
void runtime_charge_cycles(runtime_stats_t *rt, int task_id, int64_t cycles, int radio_on);
void runtime_charge_sleep(runtime_stats_t *rt, int64_t cycles);

#endif
