#include "scenario.h"

#include <ctype.h>
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

static int64_t pow10_i32(int32_t n) {
    int64_t v = 1;
    for (int32_t i = 0; i < n; i++) {
        v *= 10;
    }
    return v;
}

/* Parses decimal text into an integer scaled by `scale`, rounded to nearest. */
static int parse_decimal_scaled(const char *p, int32_t scale, int64_t *out_scaled) {
    int sign = 1;
    int64_t int_part = 0;
    int64_t frac_part = 0;
    int32_t frac_digits = 0;
    int saw_digit = 0;

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    while (isdigit((unsigned char)*p)) {
        saw_digit = 1;
        int_part = int_part * 10 + (*p - '0');
        p++;
    }

    if (*p == '.') {
        p++;
        while (isdigit((unsigned char)*p)) {
            saw_digit = 1;
            if (frac_digits < 12) {
                frac_part = frac_part * 10 + (*p - '0');
                frac_digits++;
            }
            p++;
        }
    }

    if (!saw_digit) {
        return 0;
    }

    int64_t scaled = int_part * (int64_t)scale;
    if (frac_digits > 0) {
        int64_t denom = pow10_i32(frac_digits);
        int64_t frac_scaled = (frac_part * (int64_t)scale + (denom / 2)) / denom;
        scaled += frac_scaled;
    }

    *out_scaled = (sign < 0) ? -scaled : scaled;
    return 1;
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
    int64_t scaled = 0;
    if (!parse_decimal_scaled(p, 1, &scaled)) {
        return fallback;
    }
    return (int32_t)scaled;
}

static int32_t extract_int_rounded_range(const char *start, const char *end, const char *key, int32_t fallback) {
    const char *p = strstr(start, key);
    if (!p || p >= end) {
        return fallback;
    }
    p = strchr(p, ':');
    if (!p || p >= end) {
        return fallback;
    }
    p++;
    int64_t scaled = 0;
    if (!parse_decimal_scaled(p, 1, &scaled)) {
        return fallback;
    }
    return (int32_t)scaled;
}

static int32_t extract_rad_to_mdeg_range(const char *start, const char *end, const char *key, int32_t fallback) {
    const char *p = strstr(start, key);
    if (!p || p >= end) {
        return fallback;
    }
    p = strchr(p, ':');
    if (!p || p >= end) {
        return fallback;
    }
    p++;

    int64_t rad_x1000000 = 0;
    if (!parse_decimal_scaled(p, 1000000, &rad_x1000000)) {
        return fallback;
    }

    /* mdeg = rad * (180000/pi), with integer approximation 57296 mdeg/rad. */
    int64_t mdeg = (rad_x1000000 * 57296 + ((rad_x1000000 >= 0) ? 500000 : -500000)) / 1000000;
    return (int32_t)mdeg;
}

static int32_t extract_m_to_cm_range(const char *start, const char *end, const char *key, int32_t fallback) {
    const char *p = strstr(start, key);
    if (!p || p >= end) {
        return fallback;
    }
    p = strchr(p, ':');
    if (!p || p >= end) {
        return fallback;
    }
    p++;

    int64_t cm = 0;
    if (!parse_decimal_scaled(p, 100, &cm)) {
        return fallback;
    }
    return (int32_t)cm;
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
                    int64_t omega_x1000000 = 0;
                    if (parse_decimal_scaled(omega_p + 1, 1000000, &omega_x1000000)) {
                        int64_t mdegps = (omega_x1000000 * 57296 + ((omega_x1000000 >= 0) ? 500000 : -500000)) / 1000000;
                        sc->sat_omega_mdegps = (int32_t)mdegps;
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
                        d->r_m = extract_int_range(id_pos, obj_end, "\"r\"", extract_int_rounded_range(id_pos, obj_end, "\"r\"", d->r_m));

                        d->theta_mdeg = extract_rad_to_mdeg_range(id_pos, obj_end, "\"theta\"", d->theta_mdeg);

                        d->vr_mps = extract_int_rounded_range(id_pos, obj_end, "\"vr\"", d->vr_mps);
                        d->vt_mps = extract_int_rounded_range(id_pos, obj_end, "\"vt\"", d->vt_mps);

                        d->size_cm = extract_m_to_cm_range(id_pos, obj_end, "\"size\"", d->size_cm);

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
