#include "chromapath.h"

#include <math.h>

static const double CP_DE_PI = 3.14159265358979323846264338327950288;

static double cp_degrees(double radians) {
    return radians * 180.0 / CP_DE_PI;
}

static double cp_radians(double degrees) {
    return degrees * CP_DE_PI / 180.0;
}

static double cp_hue_angle(double b, double a) {
    double angle = cp_degrees(atan2(b, a));
    return angle < 0.0 ? angle + 360.0 : angle;
}

double cp_delta_e_1976(cp_lab left, cp_lab right) {
    double dL = left.L - right.L;
    double da = left.a - right.a;
    double db = left.b - right.b;
    return sqrt(dL * dL + da * da + db * db);
}

static double cp_delta_e_1994(cp_lab left, cp_lab right,
                              double k1, double k2, double kL) {
    double c1 = hypot(left.a, left.b);
    double c2 = hypot(right.a, right.b);
    double dL = left.L - right.L;
    double dC = c1 - c2;
    double da = left.a - right.a;
    double db = left.b - right.b;
    double dH2 = da * da + db * db - dC * dC;
    if (dH2 < 0.0) dH2 = 0.0;
    double sL = 1.0;
    double sC = 1.0 + k1 * c1;
    double sH = 1.0 + k2 * c1;
    double termL = dL / kL / sL;
    double termC = dC / sC;
    double termH = sqrt(dH2) / sH;
    return sqrt(termL * termL + termC * termC + termH * termH);
}

double cp_delta_e_1994_graphics(cp_lab left, cp_lab right) {
    return cp_delta_e_1994(left, right, 0.045, 0.015, 1.0);
}

double cp_delta_e_1994_textiles(cp_lab left, cp_lab right) {
    return cp_delta_e_1994(left, right, 0.048, 0.014, 2.0);
}

double cp_delta_e_cmc(cp_lab left, cp_lab right, double lightness,
                      double chroma) {
    if (lightness <= 0.0) lightness = 1.0;
    if (chroma <= 0.0) chroma = 1.0;
    double c1 = hypot(left.a, left.b);
    double c2 = hypot(right.a, right.b);
    double dL = left.L - right.L;
    double dC = c1 - c2;
    double da = left.a - right.a;
    double db = left.b - right.b;
    double dH2 = da * da + db * db - dC * dC;
    if (dH2 < 0.0) dH2 = 0.0;
    double h1 = cp_hue_angle(left.b, left.a);
    double f = sqrt(pow(c1, 4.0) / (pow(c1, 4.0) + 1900.0));
    double t;
    if (h1 >= 164.0 && h1 <= 345.0) {
        t = 0.56 + fabs(0.2 * cos(cp_radians(h1 + 168.0)));
    } else {
        t = 0.36 + fabs(0.4 * cos(cp_radians(h1 + 35.0)));
    }
    double sL = left.L < 16.0
        ? 0.511
        : 0.040975 * left.L / (1.0 + 0.01765 * left.L);
    double sC = 0.0638 * c1 / (1.0 + 0.0131 * c1) + 0.638;
    double sH = sC * (f * t + 1.0 - f);
    double termL = dL / (lightness * sL);
    double termC = dC / (chroma * sC);
    double termH = sqrt(dH2) / sH;
    return sqrt(termL * termL + termC * termC + termH * termH);
}

double cp_delta_e_2000(cp_lab left, cp_lab right) {
    double L1 = left.L;
    double a1 = left.a;
    double b1 = left.b;
    double L2 = right.L;
    double a2 = right.a;
    double b2 = right.b;
    double C1 = hypot(a1, b1);
    double C2 = hypot(a2, b2);
    double averageC = (C1 + C2) * 0.5;
    double averageC7 = pow(averageC, 7.0);
    double g = 0.5 * (1.0 - sqrt(averageC7 / (averageC7 + pow(25.0, 7.0))));
    double adjustedA1 = (1.0 + g) * a1;
    double adjustedA2 = (1.0 + g) * a2;
    double adjustedC1 = hypot(adjustedA1, b1);
    double adjustedC2 = hypot(adjustedA2, b2);
    double adjustedH1 = cp_hue_angle(b1, adjustedA1);
    double adjustedH2 = cp_hue_angle(b2, adjustedA2);
    double deltaL = L2 - L1;
    double deltaC = adjustedC2 - adjustedC1;
    double deltaHAngle;
    if (adjustedC1 * adjustedC2 == 0.0) {
        deltaHAngle = 0.0;
    } else if (fabs(adjustedH2 - adjustedH1) <= 180.0) {
        deltaHAngle = adjustedH2 - adjustedH1;
    } else if (adjustedH2 <= adjustedH1) {
        deltaHAngle = adjustedH2 - adjustedH1 + 360.0;
    } else {
        deltaHAngle = adjustedH2 - adjustedH1 - 360.0;
    }
    double deltaH = 2.0 * sqrt(adjustedC1 * adjustedC2) *
                    sin(cp_radians(deltaHAngle * 0.5));
    double averageL = (L1 + L2) * 0.5;
    double averageAdjustedC = (adjustedC1 + adjustedC2) * 0.5;
    double averageH;
    if (adjustedC1 * adjustedC2 == 0.0) {
        averageH = adjustedH1 + adjustedH2;
    } else if (fabs(adjustedH1 - adjustedH2) <= 180.0) {
        averageH = (adjustedH1 + adjustedH2) * 0.5;
    } else if (adjustedH1 + adjustedH2 < 360.0) {
        averageH = (adjustedH1 + adjustedH2 + 360.0) * 0.5;
    } else {
        averageH = (adjustedH1 + adjustedH2 - 360.0) * 0.5;
    }
    double t = 1.0
        - 0.17 * cos(cp_radians(averageH - 30.0))
        + 0.24 * cos(cp_radians(2.0 * averageH))
        + 0.32 * cos(cp_radians(3.0 * averageH + 6.0))
        - 0.20 * cos(cp_radians(4.0 * averageH - 63.0));
    double deltaTheta = 30.0 * exp(-pow((averageH - 275.0) / 25.0, 2.0));
    double averageC7b = pow(averageAdjustedC, 7.0);
    double rC = 2.0 * sqrt(averageC7b / (averageC7b + pow(25.0, 7.0)));
    double sL = 1.0 + 0.015 * pow(averageL - 50.0, 2.0) /
                sqrt(20.0 + pow(averageL - 50.0, 2.0));
    double sC = 1.0 + 0.045 * averageAdjustedC;
    double sH = 1.0 + 0.015 * averageAdjustedC * t;
    double rT = -sin(cp_radians(2.0 * deltaTheta)) * rC;
    double termL = deltaL / sL;
    double termC = deltaC / sC;
    double termH = deltaH / sH;
    double result2 = termL * termL + termC * termC + termH * termH +
                     rT * termC * termH;
    return result2 > 0.0 ? sqrt(result2) : 0.0;
}
