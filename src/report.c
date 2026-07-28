#include "internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

cp_status cp_report_append(char *buffer, size_t capacity, size_t *used,
                           const char *format, ...) {
    if (!buffer || !used || !format || *used > capacity) return CP_ERR_ARGUMENT;
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer + *used, capacity - *used, format, args);
    va_end(args);
    if (written < 0) return CP_ERR_STATE;
    if ((size_t)written >= capacity - *used) return CP_ERR_ARGUMENT;
    *used += (size_t)written;
    return CP_OK;
}

cp_status cp_profile_format_report(const cp_profile *profile, char *buffer,
                                   size_t capacity, size_t *written) {
    if (!profile || !buffer || capacity == 0) return CP_ERR_ARGUMENT;
    size_t used = 0;
    char cls[5], cs[5], pcs[5];
    cp_signature_text(profile->parsed.header.device_class, cls);
    cp_signature_text(profile->parsed.header.color_space, cs);
    cp_signature_text(profile->parsed.header.pcs, pcs);
    cp_status st = cp_report_append(buffer, capacity, &used,
        "Profile size: %u\nClass: %s\nColor space: %s\nPCS: %s\nTags: %zu\n",
        profile->parsed.header.declared_size, cls, cs, pcs,
        profile->parsed.tag_count);
    if (st != CP_OK) return st;
    if (profile->parsed.description.text) {
        st = cp_report_append(buffer, capacity, &used, "Description: %s\n",
                              profile->parsed.description.text);
        if (st != CP_OK) return st;
    }
    if (profile->parsed.has_white_point) {
        st = cp_report_append(buffer, capacity, &used,
            "Media white point: %.6f %.6f %.6f\n",
            profile->parsed.white_point.x,
            profile->parsed.white_point.y,
            profile->parsed.white_point.z);
        if (st != CP_OK) return st;
    }
    for (size_t i = 0; i < profile->parsed.tag_count; ++i) {
        char sig[5], type[5];
        cp_signature_text(profile->parsed.tags[i].signature, sig);
        cp_signature_text(profile->parsed.tags[i].type_signature, type);
        st = cp_report_append(buffer, capacity, &used,
            "Tag %zu: %s type=%s offset=%u size=%u\n", i, sig, type,
            profile->parsed.tags[i].offset, profile->parsed.tags[i].size);
        if (st != CP_OK) return st;
    }
    if (written) *written = used;
    return CP_OK;
}
