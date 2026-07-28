#include "internal.h"

#include <math.h>
#include <string.h>

static const cp_curve *curve_for_channel(const cp_profile *profile, size_t channel) {
    if (channel == 0) return &profile->parsed.red_curve;
    if (channel == 1) return &profile->parsed.green_curve;
    return &profile->parsed.blue_curve;
}

static double curve_area(const cp_curve *curve) {
    if (!curve || curve->count == 0) return 0.5;
    if (curve->count == 1) {
        double gamma = curve->gamma > 0.0 ? curve->gamma : 1.0;
        return 1.0 / (gamma + 1.0);
    }
    if (!curve->values || curve->count < 2) return 0.5;
    double sum = 0.0;
    for (uint32_t i = 1; i < curve->count; ++i) {
        sum += (curve->values[i - 1] + curve->values[i]) * 0.5;
    }
    return sum / (double)(curve->count - 1);
}

cp_status cp_prepare_default_plan(cp_profile *profile) {
    if (!profile) return CP_ERR_ARGUMENT;
    static const char signatures[3][4] = {{'r','T','R','C'}, {'g','T','R','C'}, {'b','T','R','C'}};
    memset(&profile->parsed.default_plan, 0, sizeof(profile->parsed.default_plan));
    for (size_t channel = 0; channel < 3; ++channel) {
        profile->parsed.curve_route[channel] = (uint8_t)channel;
        const cp_curve *curve = curve_for_channel(profile, channel);
        cp_curve_view *view = &profile->parsed.default_plan.channel[channel];
        view->count = curve->count;
        view->gamma = curve->gamma;
        view->values = curve->values;
        view->source_signature = cp_signature(signatures[channel]);
        profile->parsed.default_plan.channel_area[channel] = curve_area(curve);
    }
    profile->parsed.default_plan.generation = 1;
    return CP_OK;
}
