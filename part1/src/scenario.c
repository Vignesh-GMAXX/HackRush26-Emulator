#include "scenario.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t extract_int(const char *buf, const char *key, int32_t fallback) {
    const char *p = strstr(buf, key);
    if (!p) {
        return fallback;
    }
    p = strchr(p, ':');
    if (!p) {
        return fallback;
    }
    p++;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return (int32_t)strtol(p, NULL, 10);
}

void load_default_scenario(scenario_t *sc) {
    memset(sc, 0, sizeof(*sc));
    sc->horizon_s = 180;
    sc->decision_step_s = 10;
    sc->energy_budget_mj = 150000;
    sc->sat_r_m = 6800000;
    sc->sat_theta_mdeg = 0;
    sc->sat_omega_mdegps = 60;
    sc->debris_count = 8;

    for (int i = 0; i < sc->debris_count; i++) {
        sc->debris[i].id = 100 + i;
        sc->debris[i].r_m = 6799800 + i * 40;
        sc->debris[i].theta_mdeg = 10000 + i * 4500;
        sc->debris[i].vr_mps = (i % 2 == 0) ? 1 : -1;
        sc->debris[i].vt_mps = 2 + (i % 3);
        sc->debris[i].size_cm = 10 + i;
    }
}

int load_scenario_file(const char *path, scenario_t *sc) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        load_default_scenario(sc);
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        load_default_scenario(sc);
        return -1;
    }
    long sz = ftell(fp);
    if (sz <= 0) {
        fclose(fp);
        load_default_scenario(sc);
        return -1;
    }
    rewind(fp);

    char *buf = (char *)malloc((size_t)sz + 1U);
    if (!buf) {
        fclose(fp);
        load_default_scenario(sc);
        return -1;
    }

    size_t nread = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[nread] = '\0';

    load_default_scenario(sc);

    sc->horizon_s = extract_int(buf, "\"horizon_s\"", sc->horizon_s);
    sc->decision_step_s = extract_int(buf, "\"decision_step_s\"", sc->decision_step_s);
    sc->energy_budget_mj = extract_int(buf, "\"energy_budget_mj\"", sc->energy_budget_mj);
    sc->sat_r_m = extract_int(buf, "\"sat_r_m\"", sc->sat_r_m);
    sc->sat_theta_mdeg = extract_int(buf, "\"sat_theta_mdeg\"", sc->sat_theta_mdeg);
    sc->sat_omega_mdegps = extract_int(buf, "\"sat_omega_mdegps\"", sc->sat_omega_mdegps);

    int32_t debris_count = extract_int(buf, "\"debris_count\"", sc->debris_count);
    if (debris_count > 0 && debris_count <= MAX_DEBRIS) {
        sc->debris_count = debris_count;
    }

    free(buf);
    return 0;
}
