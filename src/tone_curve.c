#include "chromapath.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct cp_tone_curve {
    double gamma;
    double *values;
    size_t count;
};

static double cp_tone_clamp(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

cp_tone_curve *cp_tone_curve_create_gamma(double gamma) {
    if (!isfinite(gamma) || gamma <= 0.0) return NULL;
    cp_tone_curve *curve = (cp_tone_curve *)calloc(1, sizeof(*curve));
    if (!curve) return NULL;
    curve->gamma = gamma;
    return curve;
}

cp_tone_curve *cp_tone_curve_create_table(const double *values, size_t count) {
    if (!values || count < 2) return NULL;
    cp_tone_curve *curve = (cp_tone_curve *)calloc(1, sizeof(*curve));
    if (!curve) return NULL;
    curve->values = (double *)malloc(count * sizeof(double));
    if (!curve->values) {
        free(curve);
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        if (!isfinite(values[i])) {
            cp_tone_curve_destroy(curve);
            return NULL;
        }
        curve->values[i] = cp_tone_clamp(values[i]);
    }
    curve->count = count;
    curve->gamma = 1.0;
    return curve;
}

void cp_tone_curve_destroy(cp_tone_curve *curve) {
    if (!curve) return;
    free(curve->values);
    free(curve);
}

double cp_tone_curve_evaluate(const cp_tone_curve *curve, double input) {
    if (!curve) return cp_tone_clamp(input);
    input = cp_tone_clamp(input);
    if (curve->count == 0) return pow(input, curve->gamma);
    if (curve->count == 1) return curve->values[0];
    double position = input * (double)(curve->count - 1);
    size_t lower = (size_t)floor(position);
    size_t upper = lower + 1;
    if (upper >= curve->count) upper = lower;
    return cp_interpolate_linear(curve->values[lower], curve->values[upper],
                                 position - (double)lower);
}

cp_status cp_tone_curve_invert(const cp_tone_curve *curve, double output,
                               double *input) {
    if (!curve || !input || !isfinite(output)) return CP_ERR_ARGUMENT;
    output = cp_tone_clamp(output);
    if (curve->count == 0) {
        *input = pow(output, 1.0 / curve->gamma);
        return CP_OK;
    }
    for (size_t i = 1; i < curve->count; ++i) {
        if (curve->values[i] + 1e-15 < curve->values[i - 1]) return CP_ERR_FORMAT;
    }
    if (output <= curve->values[0]) {
        *input = 0.0;
        return CP_OK;
    }
    if (output >= curve->values[curve->count - 1]) {
        *input = 1.0;
        return CP_OK;
    }
    size_t low = 0;
    size_t high = curve->count - 1;
    while (high - low > 1) {
        size_t middle = low + (high - low) / 2;
        if (curve->values[middle] <= output) low = middle;
        else high = middle;
    }
    double width = curve->values[high] - curve->values[low];
    double fraction = width > 0.0 ? (output - curve->values[low]) / width : 0.0;
    *input = ((double)low + fraction) / (double)(curve->count - 1);
    return CP_OK;
}

cp_status cp_tone_curve_sample(const cp_tone_curve *curve, double *values,
                               size_t count) {
    if (!curve || !values || count == 0) return CP_ERR_ARGUMENT;
    if (count == 1) {
        values[0] = cp_tone_curve_evaluate(curve, 0.0);
        return CP_OK;
    }
    for (size_t i = 0; i < count; ++i) {
        values[i] = cp_tone_curve_evaluate(curve,
                                           (double)i / (double)(count - 1));
    }
    return CP_OK;
}

size_t cp_tone_curve_sample_count(const cp_tone_curve *curve) {
    return curve ? curve->count : 0;
}
