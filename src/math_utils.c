#include "chromapath.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

static double cp_math_clamp(double value, double minimum, double maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

double cp_interpolate_linear(double left, double right, double amount) {
    return left + (right - left) * amount;
}

double cp_interpolate_cubic(double p0, double p1, double p2, double p3,
                            double amount) {
    double a0 = -0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3;
    double a1 = p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3;
    double a2 = -0.5 * p0 + 0.5 * p2;
    double a3 = p1;
    return ((a0 * amount + a1) * amount + a2) * amount + a3;
}

cp_status cp_resample_uniform(const double *source, size_t source_count,
                              double *destination, size_t destination_count) {
    if (!source || !destination || source_count == 0 || destination_count == 0) {
        return CP_ERR_ARGUMENT;
    }
    if (source_count == 1) {
        for (size_t i = 0; i < destination_count; ++i) destination[i] = source[0];
        return CP_OK;
    }
    if (destination_count == 1) {
        destination[0] = source[0];
        return CP_OK;
    }
    for (size_t i = 0; i < destination_count; ++i) {
        double position = (double)i * (double)(source_count - 1) /
                          (double)(destination_count - 1);
        size_t lower = (size_t)floor(position);
        size_t upper = lower + 1;
        if (upper >= source_count) upper = source_count - 1;
        destination[i] = cp_interpolate_linear(source[lower], source[upper],
                                               position - (double)lower);
    }
    return CP_OK;
}

cp_status cp_monotonic_cubic_slopes(const double *x, const double *y,
                                    size_t count, double *slopes) {
    if (!x || !y || !slopes || count < 2) return CP_ERR_ARGUMENT;
    double *delta = (double *)malloc((count - 1) * sizeof(double));
    if (!delta) return CP_ERR_NOMEM;
    for (size_t i = 0; i + 1 < count; ++i) {
        double width = x[i + 1] - x[i];
        if (!(width > 0.0)) {
            free(delta);
            return CP_ERR_FORMAT;
        }
        delta[i] = (y[i + 1] - y[i]) / width;
    }
    slopes[0] = delta[0];
    slopes[count - 1] = delta[count - 2];
    for (size_t i = 1; i + 1 < count; ++i) {
        if (delta[i - 1] * delta[i] <= 0.0) {
            slopes[i] = 0.0;
        } else {
            double h0 = x[i] - x[i - 1];
            double h1 = x[i + 1] - x[i];
            double w1 = 2.0 * h1 + h0;
            double w2 = h1 + 2.0 * h0;
            slopes[i] = (w1 + w2) /
                        (w1 / delta[i - 1] + w2 / delta[i]);
        }
    }
    for (size_t i = 0; i + 1 < count; ++i) {
        if (delta[i] == 0.0) {
            slopes[i] = 0.0;
            slopes[i + 1] = 0.0;
            continue;
        }
        double a = slopes[i] / delta[i];
        double b = slopes[i + 1] / delta[i];
        double magnitude = a * a + b * b;
        if (magnitude > 9.0) {
            double scale = 3.0 / sqrt(magnitude);
            slopes[i] = scale * a * delta[i];
            slopes[i + 1] = scale * b * delta[i];
        }
    }
    free(delta);
    return CP_OK;
}

double cp_monotonic_cubic_eval(const double *x, const double *y,
                               const double *slopes, size_t count, double value) {
    if (!x || !y || !slopes || count == 0) return 0.0;
    if (count == 1 || value <= x[0]) return y[0];
    if (value >= x[count - 1]) return y[count - 1];
    size_t low = 0;
    size_t high = count - 1;
    while (high - low > 1) {
        size_t middle = low + (high - low) / 2;
        if (x[middle] <= value) low = middle;
        else high = middle;
    }
    double width = x[high] - x[low];
    double t = (value - x[low]) / width;
    double t2 = t * t;
    double t3 = t2 * t;
    double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
    double h10 = t3 - 2.0 * t2 + t;
    double h01 = -2.0 * t3 + 3.0 * t2;
    double h11 = t3 - t2;
    return h00 * y[low] + h10 * width * slopes[low] +
           h01 * y[high] + h11 * width * slopes[high];
}

void cp_statistics_begin(cp_statistics_accumulator *accumulator) {
    if (!accumulator) return;
    accumulator->count = 0;
    accumulator->minimum = 0.0;
    accumulator->maximum = 0.0;
    accumulator->mean = 0.0;
    accumulator->m2 = 0.0;
    accumulator->sum = 0.0;
}

void cp_statistics_add(cp_statistics_accumulator *accumulator, double value) {
    if (!accumulator) return;
    accumulator->count += 1;
    if (accumulator->count == 1) {
        accumulator->minimum = value;
        accumulator->maximum = value;
        accumulator->mean = value;
        accumulator->m2 = 0.0;
        accumulator->sum = value;
        return;
    }
    if (value < accumulator->minimum) accumulator->minimum = value;
    if (value > accumulator->maximum) accumulator->maximum = value;
    accumulator->sum += value;
    double delta = value - accumulator->mean;
    accumulator->mean += delta / (double)accumulator->count;
    double delta2 = value - accumulator->mean;
    accumulator->m2 += delta * delta2;
}

cp_status cp_statistics_finish(const cp_statistics_accumulator *accumulator,
                               cp_statistics *out) {
    if (!accumulator || !out || accumulator->count == 0) return CP_ERR_ARGUMENT;
    out->count = accumulator->count;
    out->minimum = accumulator->minimum;
    out->maximum = accumulator->maximum;
    out->mean = accumulator->mean;
    out->sum = accumulator->sum;
    out->variance = accumulator->count > 1
        ? accumulator->m2 / (double)(accumulator->count - 1)
        : 0.0;
    out->standard_deviation = sqrt(cp_math_clamp(out->variance, 0.0, HUGE_VAL));
    return CP_OK;
}

cp_status cp_statistics_compute(const double *values, size_t count,
                                cp_statistics *out) {
    if (!values || !out || count == 0) return CP_ERR_ARGUMENT;
    cp_statistics_accumulator accumulator;
    cp_statistics_begin(&accumulator);
    for (size_t i = 0; i < count; ++i) cp_statistics_add(&accumulator, values[i]);
    return cp_statistics_finish(&accumulator, out);
}
