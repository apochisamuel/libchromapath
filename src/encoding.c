#include "chromapath.h"

#include <math.h>

static double cp_encoding_clamp(double value, double minimum, double maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

uint8_t cp_quantize_u8(double value) {
    value = cp_encoding_clamp(value, 0.0, 1.0);
    return (uint8_t)floor(value * 255.0 + 0.5);
}

uint16_t cp_quantize_u16(double value) {
    value = cp_encoding_clamp(value, 0.0, 1.0);
    return (uint16_t)floor(value * 65535.0 + 0.5);
}

double cp_dequantize_u8(uint8_t value) {
    return value / 255.0;
}

double cp_dequantize_u16(uint16_t value) {
    return value / 65535.0;
}

cp_status cp_encode_lab8(cp_lab value, uint8_t encoded[3]) {
    if (!encoded || !isfinite(value.L) || !isfinite(value.a) ||
        !isfinite(value.b)) return CP_ERR_ARGUMENT;
    double L = cp_encoding_clamp(value.L, 0.0, 100.0);
    double a = cp_encoding_clamp(value.a, -128.0, 127.0);
    double b = cp_encoding_clamp(value.b, -128.0, 127.0);
    encoded[0] = (uint8_t)floor(L * 255.0 / 100.0 + 0.5);
    encoded[1] = (uint8_t)floor(a + 128.0 + 0.5);
    encoded[2] = (uint8_t)floor(b + 128.0 + 0.5);
    return CP_OK;
}

cp_status cp_decode_lab8(const uint8_t encoded[3], cp_lab *value) {
    if (!encoded || !value) return CP_ERR_ARGUMENT;
    value->L = encoded[0] * 100.0 / 255.0;
    value->a = (double)encoded[1] - 128.0;
    value->b = (double)encoded[2] - 128.0;
    return CP_OK;
}

cp_status cp_encode_lab16(cp_lab value, uint16_t encoded[3]) {
    if (!encoded || !isfinite(value.L) || !isfinite(value.a) ||
        !isfinite(value.b)) return CP_ERR_ARGUMENT;
    double L = cp_encoding_clamp(value.L, 0.0, 100.0);
    double a = cp_encoding_clamp(value.a, -128.0, 127.99609375);
    double b = cp_encoding_clamp(value.b, -128.0, 127.99609375);
    encoded[0] = (uint16_t)floor(L * 65535.0 / 100.0 + 0.5);
    encoded[1] = (uint16_t)floor((a + 128.0) * 256.0 + 0.5);
    encoded[2] = (uint16_t)floor((b + 128.0) * 256.0 + 0.5);
    return CP_OK;
}

cp_status cp_decode_lab16(const uint16_t encoded[3], cp_lab *value) {
    if (!encoded || !value) return CP_ERR_ARGUMENT;
    value->L = encoded[0] * 100.0 / 65535.0;
    value->a = encoded[1] / 256.0 - 128.0;
    value->b = encoded[2] / 256.0 - 128.0;
    return CP_OK;
}

cp_status cp_encode_xyz16(cp_xyz value, uint16_t encoded[3]) {
    if (!encoded || !isfinite(value.x) || !isfinite(value.y) ||
        !isfinite(value.z)) return CP_ERR_ARGUMENT;
    double x = cp_encoding_clamp(value.x, 0.0, 1.999969482421875);
    double y = cp_encoding_clamp(value.y, 0.0, 1.999969482421875);
    double z = cp_encoding_clamp(value.z, 0.0, 1.999969482421875);
    encoded[0] = (uint16_t)floor(x * 32768.0 + 0.5);
    encoded[1] = (uint16_t)floor(y * 32768.0 + 0.5);
    encoded[2] = (uint16_t)floor(z * 32768.0 + 0.5);
    return CP_OK;
}

cp_status cp_decode_xyz16(const uint16_t encoded[3], cp_xyz *value) {
    if (!encoded || !value) return CP_ERR_ARGUMENT;
    value->x = encoded[0] / 32768.0;
    value->y = encoded[1] / 32768.0;
    value->z = encoded[2] / 32768.0;
    return CP_OK;
}

cp_status cp_encode_xyz16_checked(cp_xyz value, uint16_t encoded[3]) {
    if (!encoded) return CP_ERR_ARGUMENT;
    if (value.x < 0.0 || value.y < 0.0 || value.z < 0.0) return CP_ERR_FORMAT;
    if (value.x >= 2.0 || value.y >= 2.0 || value.z >= 2.0) return CP_ERR_OVERFLOW;
    return cp_encode_xyz16(value, encoded);
}

cp_status cp_decode_xyz16_checked(const uint16_t encoded[3], cp_xyz *value) {
    if (!encoded || !value) return CP_ERR_ARGUMENT;
    cp_status status = cp_decode_xyz16(encoded, value);
    if (status != CP_OK) return status;
    if (!isfinite(value->x) || !isfinite(value->y) || !isfinite(value->z)) {
        return CP_ERR_FORMAT;
    }
    return CP_OK;
}

double cp_lab_lightness_to_unit(double lightness) {
    return cp_encoding_clamp(lightness / 100.0, 0.0, 1.0);
}

double cp_lab_opponent_to_unit(double component) {
    return cp_encoding_clamp((component + 128.0) / 255.0, 0.0, 1.0);
}

double cp_lab_lightness_from_unit(double value) {
    return cp_encoding_clamp(value, 0.0, 1.0) * 100.0;
}

double cp_lab_opponent_from_unit(double value) {
    return cp_encoding_clamp(value, 0.0, 1.0) * 255.0 - 128.0;
}
