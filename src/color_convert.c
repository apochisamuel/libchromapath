#include "chromapath.h"

#include <math.h>

static const double CP_PI = 3.14159265358979323846264338327950288;

static double cp_clamp01(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

static double cp_lab_f(double value) {
    const double epsilon = 216.0 / 24389.0;
    const double kappa = 24389.0 / 27.0;
    if (value > epsilon) return cbrt(value);
    return (kappa * value + 16.0) / 116.0;
}

static double cp_lab_f_inverse(double value) {
    const double epsilon = 216.0 / 24389.0;
    const double kappa = 24389.0 / 27.0;
    double cube = value * value * value;
    if (cube > epsilon) return cube;
    return (116.0 * value - 16.0) / kappa;
}

cp_xyy cp_xyz_to_xyy(cp_xyz value) {
    cp_xyy result = {0.0, 0.0, value.y};
    double sum = value.x + value.y + value.z;
    if (sum > 0.0) {
        result.x = value.x / sum;
        result.y = value.y / sum;
    }
    return result;
}

cp_xyz cp_xyy_to_xyz(cp_xyy value) {
    cp_xyz result = {0.0, value.Y, 0.0};
    if (value.y > 0.0) {
        result.x = value.x * value.Y / value.y;
        result.z = (1.0 - value.x - value.y) * value.Y / value.y;
    }
    return result;
}

cp_lab cp_xyz_to_lab(cp_xyz value, cp_xyz white_point) {
    cp_lab result = {0.0, 0.0, 0.0};
    if (white_point.x <= 0.0 || white_point.y <= 0.0 || white_point.z <= 0.0) {
        return result;
    }
    double fx = cp_lab_f(value.x / white_point.x);
    double fy = cp_lab_f(value.y / white_point.y);
    double fz = cp_lab_f(value.z / white_point.z);
    result.L = 116.0 * fy - 16.0;
    result.a = 500.0 * (fx - fy);
    result.b = 200.0 * (fy - fz);
    return result;
}

cp_xyz cp_lab_to_xyz(cp_lab value, cp_xyz white_point) {
    double fy = (value.L + 16.0) / 116.0;
    double fx = fy + value.a / 500.0;
    double fz = fy - value.b / 200.0;
    cp_xyz result;
    result.x = white_point.x * cp_lab_f_inverse(fx);
    result.y = white_point.y * cp_lab_f_inverse(fy);
    result.z = white_point.z * cp_lab_f_inverse(fz);
    return result;
}

cp_lch cp_lab_to_lch(cp_lab value) {
    cp_lch result;
    result.L = value.L;
    result.C = hypot(value.a, value.b);
    result.h = atan2(value.b, value.a) * 180.0 / CP_PI;
    if (result.h < 0.0) result.h += 360.0;
    return result;
}

cp_lab cp_lch_to_lab(cp_lch value) {
    double radians = value.h * CP_PI / 180.0;
    cp_lab result;
    result.L = value.L;
    result.a = value.C * cos(radians);
    result.b = value.C * sin(radians);
    return result;
}

static double cp_srgb_decode(double value) {
    value = cp_clamp01(value);
    if (value <= 0.04045) return value / 12.92;
    return pow((value + 0.055) / 1.055, 2.4);
}

static double cp_srgb_encode(double value) {
    if (value <= 0.0031308) return cp_clamp01(12.92 * value);
    return cp_clamp01(1.055 * pow(value, 1.0 / 2.4) - 0.055);
}

cp_xyz cp_srgb_to_xyz(cp_rgb value) {
    double r = cp_srgb_decode(value.r);
    double g = cp_srgb_decode(value.g);
    double b = cp_srgb_decode(value.b);
    cp_xyz result;
    result.x = 0.4124564 * r + 0.3575761 * g + 0.1804375 * b;
    result.y = 0.2126729 * r + 0.7151522 * g + 0.0721750 * b;
    result.z = 0.0193339 * r + 0.1191920 * g + 0.9503041 * b;
    return result;
}

cp_rgb cp_xyz_to_srgb(cp_xyz value) {
    double r = 3.2404542 * value.x - 1.5371385 * value.y - 0.4985314 * value.z;
    double g = -0.9692660 * value.x + 1.8760108 * value.y + 0.0415560 * value.z;
    double b = 0.0556434 * value.x - 0.2040259 * value.y + 1.0572252 * value.z;
    cp_rgb result = {cp_srgb_encode(r), cp_srgb_encode(g), cp_srgb_encode(b)};
    return result;
}

cp_hsl cp_rgb_to_hsl(cp_rgb value) {
    double r = cp_clamp01(value.r);
    double g = cp_clamp01(value.g);
    double b = cp_clamp01(value.b);
    double maximum = fmax(r, fmax(g, b));
    double minimum = fmin(r, fmin(g, b));
    double delta = maximum - minimum;
    cp_hsl result = {0.0, 0.0, (maximum + minimum) * 0.5};
    if (delta == 0.0) return result;
    result.s = result.l > 0.5
        ? delta / (2.0 - maximum - minimum)
        : delta / (maximum + minimum);
    if (maximum == r) result.h = (g - b) / delta + (g < b ? 6.0 : 0.0);
    else if (maximum == g) result.h = (b - r) / delta + 2.0;
    else result.h = (r - g) / delta + 4.0;
    result.h *= 60.0;
    return result;
}

static double cp_hue_to_rgb(double p, double q, double t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

cp_rgb cp_hsl_to_rgb(cp_hsl value) {
    double h = fmod(value.h, 360.0) / 360.0;
    if (h < 0.0) h += 1.0;
    double s = cp_clamp01(value.s);
    double l = cp_clamp01(value.l);
    if (s == 0.0) return (cp_rgb){l, l, l};
    double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
    double p = 2.0 * l - q;
    return (cp_rgb){cp_hue_to_rgb(p, q, h + 1.0 / 3.0),
                    cp_hue_to_rgb(p, q, h),
                    cp_hue_to_rgb(p, q, h - 1.0 / 3.0)};
}

cp_hsv cp_rgb_to_hsv(cp_rgb value) {
    double r = cp_clamp01(value.r);
    double g = cp_clamp01(value.g);
    double b = cp_clamp01(value.b);
    double maximum = fmax(r, fmax(g, b));
    double minimum = fmin(r, fmin(g, b));
    double delta = maximum - minimum;
    cp_hsv result = {0.0, maximum == 0.0 ? 0.0 : delta / maximum, maximum};
    if (delta == 0.0) return result;
    if (maximum == r) result.h = 60.0 * fmod((g - b) / delta, 6.0);
    else if (maximum == g) result.h = 60.0 * ((b - r) / delta + 2.0);
    else result.h = 60.0 * ((r - g) / delta + 4.0);
    if (result.h < 0.0) result.h += 360.0;
    return result;
}

cp_rgb cp_hsv_to_rgb(cp_hsv value) {
    double h = fmod(value.h, 360.0);
    if (h < 0.0) h += 360.0;
    double s = cp_clamp01(value.s);
    double v = cp_clamp01(value.v);
    double c = v * s;
    double x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    if (h < 60.0) { r = c; g = x; }
    else if (h < 120.0) { r = x; g = c; }
    else if (h < 180.0) { g = c; b = x; }
    else if (h < 240.0) { g = x; b = c; }
    else if (h < 300.0) { r = x; b = c; }
    else { r = c; b = x; }
    return (cp_rgb){r + m, g + m, b + m};
}

cp_cmyk cp_rgb_to_cmyk(cp_rgb value) {
    double r = cp_clamp01(value.r);
    double g = cp_clamp01(value.g);
    double b = cp_clamp01(value.b);
    double k = 1.0 - fmax(r, fmax(g, b));
    if (k >= 1.0 - 1e-15) return (cp_cmyk){0.0, 0.0, 0.0, 1.0};
    return (cp_cmyk){(1.0 - r - k) / (1.0 - k),
                     (1.0 - g - k) / (1.0 - k),
                     (1.0 - b - k) / (1.0 - k), k};
}

cp_rgb cp_cmyk_to_rgb(cp_cmyk value) {
    double c = cp_clamp01(value.c);
    double m = cp_clamp01(value.m);
    double y = cp_clamp01(value.y);
    double k = cp_clamp01(value.k);
    return (cp_rgb){(1.0 - c) * (1.0 - k),
                    (1.0 - m) * (1.0 - k),
                    (1.0 - y) * (1.0 - k)};
}
