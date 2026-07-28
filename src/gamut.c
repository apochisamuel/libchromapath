#include "internal.h"

#include <float.h>
#include <math.h>

cp_status cp_gamut_analyze_rgb_transform(const cp_transform *transform,
                                         size_t steps, cp_gamut_summary *out) {
    if (!transform || !out || steps < 2 || steps > 65) return CP_ERR_ARGUMENT;
    cp_lab minimum = {DBL_MAX, DBL_MAX, DBL_MAX};
    cp_lab maximum = {-DBL_MAX, -DBL_MAX, -DBL_MAX};
    cp_lab total = {0.0, 0.0, 0.0};
    size_t count = 0;
    for (size_t r = 0; r < steps; ++r) {
        for (size_t g = 0; g < steps; ++g) {
            for (size_t b = 0; b < steps; ++b) {
                double input[3] = {(double)r / (double)(steps - 1),
                                   (double)g / (double)(steps - 1),
                                   (double)b / (double)(steps - 1)};
                double output[3];
                cp_status status = cp_transform_apply_rgb(transform, input, output);
                if (status != CP_OK) return status;
                cp_lab lab = cp_xyz_to_lab((cp_xyz){output[0], output[1], output[2]},
                                           transform->white_point);
                if (lab.L < minimum.L) minimum.L = lab.L;
                if (lab.a < minimum.a) minimum.a = lab.a;
                if (lab.b < minimum.b) minimum.b = lab.b;
                if (lab.L > maximum.L) maximum.L = lab.L;
                if (lab.a > maximum.a) maximum.a = lab.a;
                if (lab.b > maximum.b) maximum.b = lab.b;
                total.L += lab.L;
                total.a += lab.a;
                total.b += lab.b;
                count += 1;
            }
        }
    }
    out->minimum = minimum;
    out->maximum = maximum;
    out->average.L = total.L / (double)count;
    out->average.a = total.a / (double)count;
    out->average.b = total.b / (double)count;
    out->volume_estimate = (maximum.L - minimum.L) *
                           (maximum.a - minimum.a) *
                           (maximum.b - minimum.b);
    out->sample_count = count;
    return CP_OK;
}

cp_status cp_gamut_contains_lab(const cp_gamut_summary *summary, cp_lab value,
                                int *inside) {
    if (!summary || !inside) return CP_ERR_ARGUMENT;
    *inside = value.L >= summary->minimum.L && value.L <= summary->maximum.L &&
              value.a >= summary->minimum.a && value.a <= summary->maximum.a &&
              value.b >= summary->minimum.b && value.b <= summary->maximum.b;
    return CP_OK;
}
