#include "internal.h"

#include <stdlib.h>
#include <string.h>

static void matrix_identity(cp_matrix3 *m) {
    memset(m, 0, sizeof(*m));
    m->v[0][0] = 1.0;
    m->v[1][1] = 1.0;
    m->v[2][2] = 1.0;
}

cp_status cp_transform_create(const cp_profile *profile, uint32_t intent,
                              cp_transform **out_transform) {
    if (!profile || !out_transform) return CP_ERR_ARGUMENT;
    *out_transform = NULL;
    cp_transform *transform = (cp_transform *)calloc(1, sizeof(*transform));
    if (!transform) return CP_ERR_NOMEM;
    transform->intent = intent;
    transform->white_point = profile->parsed.has_white_point
        ? profile->parsed.white_point : (cp_xyz){0.9642, 1.0, 0.8249};
    if (profile->parsed.has_matrix) transform->matrix = profile->parsed.rgb_to_xyz;
    else matrix_identity(&transform->matrix);
    cp_status st = cp_curve_copy_view(&profile->parsed.default_plan.channel[0],
                                      &transform->red_curve);
    if (st == CP_OK) {
        st = cp_curve_copy_view(&profile->parsed.default_plan.channel[1],
                                &transform->green_curve);
    }
    if (st == CP_OK) {
        st = cp_curve_copy_view(&profile->parsed.default_plan.channel[2],
                                &transform->blue_curve);
    }
    if (st != CP_OK) {
        cp_transform_destroy(transform);
        return st;
    }
    *out_transform = transform;
    return CP_OK;
}

void cp_transform_destroy(cp_transform *transform) {
    if (!transform) return;
    cp_curve_destroy(&transform->red_curve);
    cp_curve_destroy(&transform->green_curve);
    cp_curve_destroy(&transform->blue_curve);
    free(transform);
}

cp_status cp_transform_apply_rgb(const cp_transform *transform,
                                 const double input[3], double output[3]) {
    if (!transform || !input || !output) return CP_ERR_ARGUMENT;
    double linear[3] = {
        cp_curve_eval(&transform->red_curve, input[0]),
        cp_curve_eval(&transform->green_curve, input[1]),
        cp_curve_eval(&transform->blue_curve, input[2])
    };
    for (size_t row = 0; row < 3; ++row) {
        output[row] = transform->matrix.v[row][0] * linear[0] +
                      transform->matrix.v[row][1] * linear[1] +
                      transform->matrix.v[row][2] * linear[2];
    }
    return CP_OK;
}

cp_status cp_transform_apply_rgb_batch(const cp_transform *transform,
                                       const double *input, double *output,
                                       size_t pixel_count, size_t stride) {
    if (!transform || !input || !output || stride < 3) return CP_ERR_ARGUMENT;
    for (size_t pixel = 0; pixel < pixel_count; ++pixel) {
        cp_status status = cp_transform_apply_rgb(transform,
                                                  input + pixel * stride,
                                                  output + pixel * stride);
        if (status != CP_OK) return status;
    }
    return CP_OK;
}
