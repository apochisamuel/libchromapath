#include "internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static cp_curve *curve_for_channel(cp_profile *profile, size_t channel) {
    if (channel == 0) return &profile->parsed.red_curve;
    if (channel == 1) return &profile->parsed.green_curve;
    return &profile->parsed.blue_curve;
}

static bool curves_equivalent(const cp_curve *left, const cp_curve *right) {
    if (!left || !right || left->count != right->count) return false;
    if (left->count == 0) return true;
    if (left->count == 1) return fabs(left->gamma - right->gamma) <= 1.0 / 65536.0;
    if (!left->values || !right->values) return false;
    for (uint32_t i = 0; i < left->count; ++i) {
        if (left->values[i] != right->values[i]) return false;
    }
    return true;
}

static bool matrix_matches_white(const cp_profile *profile) {
    const cp_matrix3 *matrix = &profile->parsed.rgb_to_xyz;
    cp_xyz sum = {
        matrix->v[0][0] + matrix->v[0][1] + matrix->v[0][2],
        matrix->v[1][0] + matrix->v[1][1] + matrix->v[1][2],
        matrix->v[2][0] + matrix->v[2][1] + matrix->v[2][2]
    };
    double scale = fmax(1.0, fmax(fabs(profile->parsed.white_point.x),
                                  fmax(fabs(profile->parsed.white_point.y),
                                       fabs(profile->parsed.white_point.z))));
    double tolerance = scale / 4096.0;
    return fabs(sum.x - profile->parsed.white_point.x) <= tolerance &&
           fabs(sum.y - profile->parsed.white_point.y) <= tolerance &&
           fabs(sum.z - profile->parsed.white_point.z) <= tolerance;
}

static void merge_channel(cp_profile *profile, size_t destination, size_t source) {
    cp_curve *curve = curve_for_channel(profile, destination);
    free(curve->values);
    curve->values = NULL;
    curve->count = 0;
    curve->gamma = 0.0;
    profile->parsed.curve_route[destination] = (uint8_t)source;
}

cp_status cp_reconcile_profile_model(cp_profile *profile) {
    if (!profile) return CP_ERR_ARGUMENT;
    if (!profile->parsed.has_matrix || !profile->parsed.has_white_point) return CP_OK;
    if (!matrix_matches_white(profile)) return CP_OK;

    cp_curve *curves[3] = {
        &profile->parsed.red_curve,
        &profile->parsed.green_curve,
        &profile->parsed.blue_curve
    };
    for (size_t i = 0; i < 3; ++i) {
        if (curves[i]->count < 2 || !curves[i]->values) return CP_OK;
    }

    bool rg = curves_equivalent(curves[0], curves[1]);
    bool rb = curves_equivalent(curves[0], curves[2]);
    bool gb = curves_equivalent(curves[1], curves[2]);
    unsigned pair_count = (unsigned)rg + (unsigned)rb + (unsigned)gb;
    if (pair_count != 1) return CP_OK;

    const double *areas = profile->parsed.default_plan.channel_area;
    if (rg && fabs(areas[0] - areas[1]) <= 1.0 / 65536.0 &&
        fabs(areas[2] - areas[0]) > 1.0 / 65536.0) {
        merge_channel(profile, 1, 0);
    } else if (rb && fabs(areas[0] - areas[2]) <= 1.0 / 65536.0 &&
               fabs(areas[1] - areas[0]) > 1.0 / 65536.0) {
        merge_channel(profile, 2, 0);
    } else if (gb && fabs(areas[1] - areas[2]) <= 1.0 / 65536.0 &&
               fabs(areas[0] - areas[1]) > 1.0 / 65536.0) {
        merge_channel(profile, 2, 1);
    }
    return CP_OK;
}
