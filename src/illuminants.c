#include "chromapath.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>

static const cp_illuminant_info cp_illuminants[] = {
    {
        "A",
        {0.447570, 0.407450, 1.000000},
        2856.000000
    },
    {
        "B",
        {0.348420, 0.351610, 1.000000},
        4874.000000
    },
    {
        "C",
        {0.310060, 0.316160, 1.000000},
        6774.000000
    },
    {
        "D50",
        {0.345670, 0.358500, 1.000000},
        5003.000000
    },
    {
        "D55",
        {0.332420, 0.347430, 1.000000},
        5503.000000
    },
    {
        "D60",
        {0.321680, 0.337670, 1.000000},
        6000.000000
    },
    {
        "D65",
        {0.312710, 0.329020, 1.000000},
        6504.000000
    },
    {
        "D75",
        {0.299020, 0.314850, 1.000000},
        7504.000000
    },
    {
        "E",
        {0.333333, 0.333333, 1.000000},
        5454.000000
    },
    {
        "F1",
        {0.313100, 0.337100, 1.000000},
        6430.000000
    },
    {
        "F2",
        {0.372100, 0.375100, 1.000000},
        4230.000000
    },
    {
        "F3",
        {0.409100, 0.394100, 1.000000},
        3450.000000
    },
    {
        "F4",
        {0.440200, 0.403100, 1.000000},
        2940.000000
    },
    {
        "F5",
        {0.313800, 0.345200, 1.000000},
        6350.000000
    },
    {
        "F6",
        {0.377900, 0.388200, 1.000000},
        4150.000000
    },
    {
        "F7",
        {0.312900, 0.329200, 1.000000},
        6500.000000
    },
    {
        "F8",
        {0.345800, 0.358600, 1.000000},
        5000.000000
    },
    {
        "F9",
        {0.374100, 0.372700, 1.000000},
        4150.000000
    },
    {
        "F10",
        {0.345800, 0.358800, 1.000000},
        5000.000000
    },
    {
        "F11",
        {0.380500, 0.376900, 1.000000},
        4000.000000
    },
    {
        "F12",
        {0.437000, 0.404200, 1.000000},
        3000.000000
    },
    {
        "ICC PCS",
        {0.345703, 0.358539, 1.000000},
        5000.000000
    }
};

static int cp_illuminant_name_equal(const char *left, const char *right) {
    if (!left || !right) return 0;
    while (*left && *right) {
        unsigned char a = (unsigned char)*left++;
        unsigned char b = (unsigned char)*right++;
        if (tolower(a) != tolower(b)) return 0;
    }
    return *left == 0 && *right == 0;
}

size_t cp_illuminant_count(void) {
    return sizeof(cp_illuminants) / sizeof(cp_illuminants[0]);
}

cp_status cp_illuminant_at(size_t index, cp_illuminant_info *out) {
    if (!out) return CP_ERR_ARGUMENT;
    if (index >= cp_illuminant_count()) return CP_ERR_NOT_FOUND;
    *out = cp_illuminants[index];
    return CP_OK;
}

cp_status cp_illuminant_find(const char *name, cp_illuminant_info *out) {
    if (!name || !out) return CP_ERR_ARGUMENT;
    for (size_t i = 0; i < cp_illuminant_count(); ++i) {
        if (cp_illuminant_name_equal(name, cp_illuminants[i].name)) {
            *out = cp_illuminants[i];
            return CP_OK;
        }
    }
    return CP_ERR_NOT_FOUND;
}

cp_xyz cp_illuminant_xyz(const cp_illuminant_info *illuminant) {
    if (!illuminant) return (cp_xyz){0.0, 0.0, 0.0};
    return cp_xyy_to_xyz(illuminant->chromaticity);
}

double cp_xyy_correlated_temperature(cp_xyy chromaticity) {
    double denominator = 0.1858 - chromaticity.y;
    if (fabs(denominator) < 1e-12) return 0.0;
    double n = (chromaticity.x - 0.3320) / denominator;
    return 449.0 * n * n * n + 3525.0 * n * n + 6823.3 * n + 5520.33;
}

cp_xyy cp_daylight_chromaticity(double correlated_temperature) {
    if (correlated_temperature < 4000.0) correlated_temperature = 4000.0;
    if (correlated_temperature > 25000.0) correlated_temperature = 25000.0;
    double t = correlated_temperature;
    double x;
    if (t <= 7000.0) {
        x = -4.6070e9 / (t * t * t) + 2.9678e6 / (t * t) +
            0.09911e3 / t + 0.244063;
    } else {
        x = -2.0064e9 / (t * t * t) + 1.9018e6 / (t * t) +
            0.24748e3 / t + 0.237040;
    }
    double y = -3.0 * x * x + 2.870 * x - 0.275;
    return (cp_xyy){x, y, 1.0};
}

cp_xyy cp_blackbody_chromaticity(double temperature_kelvin) {
    double t = temperature_kelvin;
    if (t < 1667.0) t = 1667.0;
    if (t > 25000.0) t = 25000.0;
    double x;
    if (t <= 4000.0) {
        x = -0.2661239e9 / (t * t * t) - 0.2343580e6 / (t * t) +
            0.8776956e3 / t + 0.179910;
    } else {
        x = -3.0258469e9 / (t * t * t) + 2.1070379e6 / (t * t) +
            0.2226347e3 / t + 0.240390;
    }
    double y;
    if (t <= 2222.0) {
        y = -1.1063814 * x * x * x - 1.34811020 * x * x +
            2.18555832 * x - 0.20219683;
    } else if (t <= 4000.0) {
        y = -0.9549476 * x * x * x - 1.37418593 * x * x +
            2.09137015 * x - 0.16748867;
    } else {
        y = 3.0817580 * x * x * x - 5.87338670 * x * x +
            3.75112997 * x - 0.37001483;
    }
    return (cp_xyy){x, y, 1.0};
}
