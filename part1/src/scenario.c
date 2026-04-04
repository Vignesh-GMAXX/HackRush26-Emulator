#include "scenario.h"

#include <math.h>
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

static int32_t extract_int_range(const char *start, const char *end, const char *key, int32_t fallback) {
    const char *p = strstr(start, key);
    if (!p || p >= end) {
        return fallback;
    }
    p = strchr(p, ':');
    if (!p || p >= end) {
        return fallback;
    }
    p++;
    return (int32_t)strtol(p, NULL, 10);
}

static double extract_double_range(const char *start, const char *end, const char *key, double fallback) {
    const char *p = strstr(start, key);
    if (!p || p >= end) {
        return fallback;
    }
    p = strchr(p, ':');
    if (!p || p >= end) {
        return fallback;
    }
    p++;
    return strtod(p, NULL);
}

static int32_t extract_int_rounded(const char *buf, const char *key, int32_t fallback) {
    const char *p = strstr(buf, key);
    if (!p) {
        return fallback;
    }
    p = strchr(p, ':');
    if (!p) {
        return fallback;
    }
    p++;
    double v = strtod(p, NULL);
    if (v >= 0.0) {
        return (int32_t)(v + 0.5);
    }
    return (int32_t)(v - 0.5);
}

int load_scenario_file(const char *path, scenario_t *sc) {
    memset(sc, 0, sizeof(*sc));

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    long sz = ftell(fp);
    if (sz <= 0) {
        fclose(fp);
        return -1;
    }
    rewind(fp);

    char *buf = (char *)malloc((size_t)sz + 1U);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    size_t nread = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[nread] = '\0';

    sc->horizon_s = extract_int(buf, "\"horizon_s\"", sc->horizon_s);
    sc->decision_step_s = extract_int(buf, "\"decision_step_s\"", sc->decision_step_s);
    sc->energy_budget_mj = extract_int(buf, "\"energy_budget_mj\"", sc->energy_budget_mj);
    sc->sat_r_m = extract_int(buf, "\"sat_r_m\"", sc->sat_r_m);
    sc->sat_theta_mdeg = extract_int(buf, "\"sat_theta_mdeg\"", sc->sat_theta_mdeg);
    sc->sat_omega_mdegps = extract_int(buf, "\"sat_omega_mdegps\"", sc->sat_omega_mdegps);

    /* Conic dataset format support (nested JSON). */
    {
        int32_t energy_budget_j = extract_int_rounded(buf, "\"energy_budget_J\"", -1);
        if (energy_budget_j >= 0) {
            sc->energy_budget_mj = energy_budget_j * 1000;
        }

        sc->sat_r_m = extract_int_rounded(buf, "\"radius_m\"", sc->sat_r_m);
        {
            const char *omega_p = strstr(buf, "\"angular_velocity_rad_s\"");
            if (omega_p) {
                omega_p = strchr(omega_p, ':');
                if (omega_p) {
                    double omega_rad_s = strtod(omega_p + 1, NULL);
                    double omega_mdegps = omega_rad_s * (180000.0 / 3.14159265358979323846);
                    if (omega_mdegps >= 0.0) {
                        sc->sat_omega_mdegps = (int32_t)(omega_mdegps + 0.5);
                    } else {
                        sc->sat_omega_mdegps = (int32_t)(omega_mdegps - 0.5);
                    }
                }
            }
        }

        {
            const char *catalog = strstr(buf, "\"debris_catalog\"");
            if (catalog) {
                const char *arr_start = strchr(catalog, '[');
                const char *arr_end = arr_start ? strchr(arr_start, ']') : NULL;
                if (arr_start && arr_end && arr_end > arr_start) {
                    int parsed = 0;
                    const char *cursor = arr_start;
                    while (parsed < MAX_DEBRIS) {
                        const char *id_pos = strstr(cursor, "\"id\"");
                        if (!id_pos || id_pos >= arr_end) {
                            break;
                        }

                        const char *next_id = strstr(id_pos + 4, "\"id\"");
                        const char *obj_end = (next_id && next_id < arr_end) ? next_id : arr_end;

                        debris_t *d = &sc->debris[parsed];
                        int32_t fallback_id = d->id;
                        d->id = extract_int_range(id_pos, obj_end, "\"id\"", fallback_id);
                        d->r_m = extract_int_range(id_pos, obj_end, "\"r\"", extract_int_rounded(id_pos, "\"r\"", d->r_m));

                        {
                            double theta_rad = extract_double_range(id_pos, obj_end, "\"theta\"", -1.0);
                            if (theta_rad >= 0.0) {
                                double theta_mdeg = theta_rad * (180000.0 / 3.14159265358979323846);
                                d->theta_mdeg = (int32_t)(theta_mdeg + 0.5);
                            }
                        }

                        d->vr_mps = extract_int_rounded(id_pos, "\"vr\"", d->vr_mps);
                        d->vt_mps = extract_int_rounded(id_pos, "\"vt\"", d->vt_mps);

                        {
                            double size_m = extract_double_range(id_pos, obj_end, "\"size\"", -1.0);
                            if (size_m >= 0.0) {
                                double size_cm = size_m * 100.0;
                                d->size_cm = (int32_t)(size_cm + 0.5);
                            }
                        }

                        parsed++;
                        cursor = obj_end;
                    }

                    if (parsed > 0) {
                        sc->debris_count = parsed;
                    }
                }
            }
        }
    }

    int32_t debris_count = extract_int(buf, "\"debris_count\"", sc->debris_count);
    if (debris_count >= 0 && debris_count <= MAX_DEBRIS) {
        sc->debris_count = debris_count;
    }

    /* Reject malformed scenarios instead of silently using fabricated defaults. */
    if (sc->decision_step_s <= 0 || sc->energy_budget_mj <= 0 || sc->sat_r_m <= 0 ||
        sc->debris_count < 0 || sc->debris_count > MAX_DEBRIS) {
        free(buf);
        memset(sc, 0, sizeof(*sc));
        return -1;
    }

    free(buf);
    return 0;
}
