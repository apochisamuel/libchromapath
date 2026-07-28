#include "internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void cp_curve_destroy(cp_curve *curve) {
    if (!curve) return;
    free(curve->values);
    memset(curve, 0, sizeof(*curve));
}

cp_status cp_curve_copy(const cp_curve *source, cp_curve *destination) {
    if (!source || !destination) return CP_ERR_ARGUMENT;
    memset(destination, 0, sizeof(*destination));
    destination->count = source->count;
    destination->gamma = source->gamma;
    if (source->count && source->values) {
        destination->values = (double *)malloc(sizeof(double) * source->count);
        if (!destination->values) return CP_ERR_NOMEM;
        memcpy(destination->values, source->values, sizeof(double) * source->count);
    }
    return CP_OK;
}

cp_status cp_curve_copy_view(const cp_curve_view *source, cp_curve *destination) {
    if (!source || !destination) return CP_ERR_ARGUMENT;
    memset(destination, 0, sizeof(*destination));
    destination->count = source->count;
    destination->gamma = source->gamma;
    if (source->count > 1) {
        if (!source->values) return CP_ERR_STATE;
        destination->values = (double *)malloc(sizeof(double) * source->count);
        if (!destination->values) return CP_ERR_NOMEM;
        memcpy(destination->values, source->values, sizeof(double) * source->count);
    }
    return CP_OK;
}

double cp_curve_eval(const cp_curve *curve, double value) {
    if (!curve) return value;
    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;
    if (curve->count == 0) return value;
    if (curve->count == 1) return pow(value, curve->gamma > 0.0 ? curve->gamma : 1.0);
    if (!curve->values) return value;
    double position = value * (double)(curve->count - 1);
    size_t low = (size_t)position;
    size_t high = low + 1;
    if (high >= curve->count) return curve->values[curve->count - 1];
    double fraction = position - (double)low;
    return curve->values[low] * (1.0 - fraction) + curve->values[high] * fraction;
}

cp_status cp_decode_curve_tag(cp_profile *profile, const cp_tag_record *tag,
                              cp_curve *out) {
    if (!profile || !tag || !out) return CP_ERR_ARGUMENT;
    memset(out, 0, sizeof(*out));
    cp_reader reader;
    cp_reader_init(&reader, profile->storage + tag->offset, tag->size);
    uint32_t type, reserved, count;
    if (!cp_read_u32be(&reader, &type) || !cp_read_u32be(&reader, &reserved) ||
        !cp_read_u32be(&reader, &count)) return CP_ERR_TRUNCATED;
    (void)reserved;
    if (type != cp_signature("curv")) return CP_ERR_UNSUPPORTED;
    if (count > (tag->size - 12) / 2) return CP_ERR_TRUNCATED;
    out->count = count;
    if (count == 1) {
        uint16_t gamma;
        if (!cp_read_u16be(&reader, &gamma)) return CP_ERR_TRUNCATED;
        out->gamma = (double)gamma / 256.0;
        return CP_OK;
    }
    if (count > 0) {
        out->values = (double *)calloc(count, sizeof(double));
        if (!out->values) return CP_ERR_NOMEM;
        for (uint32_t i = 0; i < count; ++i) {
            uint16_t sample;
            if (!cp_read_u16be(&reader, &sample)) {
                cp_curve_destroy(out);
                return CP_ERR_TRUNCATED;
            }
            out->values[i] = (double)sample / 65535.0;
        }
    }
    return CP_OK;
}
