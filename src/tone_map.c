#include "chromapath.h"

#include <math.h>

static double cp_tm_clamp(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

double cp_tone_map_reinhard(double value, double white) {
    if (value <= 0.0) return 0.0;
    if (white > 0.0) {
        double white2 = white * white;
        return value * (1.0 + value / white2) / (1.0 + value);
    }
    return value / (1.0 + value);
}

double cp_tone_map_hable(double value) {
    const double A = 0.15;
    const double B = 0.50;
    const double C = 0.10;
    const double D = 0.20;
    const double E = 0.02;
    const double F = 0.30;
    if (value <= 0.0) return 0.0;
    double numerator = value * (A * value + C * B) + D * E;
    double denominator = value * (A * value + B) + D * F;
    return numerator / denominator - E / F;
}

double cp_tone_map_aces(double value) {
    if (value <= 0.0) return 0.0;
    const double a = 2.51;
    const double b = 0.03;
    const double c = 2.43;
    const double d = 0.59;
    const double e = 0.14;
    return cp_tm_clamp((value * (a * value + b)) /
                       (value * (c * value + d) + e));
}

cp_rgb cp_tone_map_rgb(cp_rgb value, double exposure, int operator_id) {
    double multiplier = pow(2.0, exposure);
    double channels[3] = {value.r * multiplier,
                          value.g * multiplier,
                          value.b * multiplier};
    for (size_t i = 0; i < 3; ++i) {
        if (operator_id == 1) channels[i] = cp_tone_map_hable(channels[i]);
        else if (operator_id == 2) channels[i] = cp_tone_map_aces(channels[i]);
        else channels[i] = cp_tone_map_reinhard(channels[i], 4.0);
    }
    return (cp_rgb){cp_tm_clamp(channels[0]),
                    cp_tm_clamp(channels[1]),
                    cp_tm_clamp(channels[2])};
}
