#include "internal.h"

#include <stdio.h>
#include <string.h>

static cp_status cp_json_append_escaped(char *buffer, size_t capacity,
                                        size_t *used, const char *text) {
    cp_status status = cp_report_append(buffer, capacity, used, "\"");
    if (status != CP_OK) return status;
    for (const unsigned char *cursor = (const unsigned char *)text; *cursor; ++cursor) {
        unsigned char value = *cursor;
        if (value == '"' || value == '\\') {
            status = cp_report_append(buffer, capacity, used, "\\%c", value);
        } else if (value == '\b') {
            status = cp_report_append(buffer, capacity, used, "\\b");
        } else if (value == '\f') {
            status = cp_report_append(buffer, capacity, used, "\\f");
        } else if (value == '\n') {
            status = cp_report_append(buffer, capacity, used, "\\n");
        } else if (value == '\r') {
            status = cp_report_append(buffer, capacity, used, "\\r");
        } else if (value == '\t') {
            status = cp_report_append(buffer, capacity, used, "\\t");
        } else if (value < 0x20) {
            status = cp_report_append(buffer, capacity, used, "\\u%04x", value);
        } else {
            status = cp_report_append(buffer, capacity, used, "%c", value);
        }
        if (status != CP_OK) return status;
    }
    return cp_report_append(buffer, capacity, used, "\"");
}

cp_status cp_profile_format_json(const cp_profile *profile, char *buffer,
                                 size_t capacity, size_t *written) {
    if (!profile || !buffer || capacity == 0) return CP_ERR_ARGUMENT;
    size_t used = 0;
    char device_class[5];
    char color_space[5];
    char pcs[5];
    cp_signature_text(profile->parsed.header.device_class, device_class);
    cp_signature_text(profile->parsed.header.color_space, color_space);
    cp_signature_text(profile->parsed.header.pcs, pcs);
    cp_status status = cp_report_append(buffer, capacity, &used,
        "{\n  \"declared_size\": %u,\n  \"device_class\": ",
        profile->parsed.header.declared_size);
    if (status != CP_OK) return status;
    status = cp_json_append_escaped(buffer, capacity, &used, device_class);
    if (status != CP_OK) return status;
    status = cp_report_append(buffer, capacity, &used, ",\n  \"color_space\": ");
    if (status != CP_OK) return status;
    status = cp_json_append_escaped(buffer, capacity, &used, color_space);
    if (status != CP_OK) return status;
    status = cp_report_append(buffer, capacity, &used, ",\n  \"pcs\": ");
    if (status != CP_OK) return status;
    status = cp_json_append_escaped(buffer, capacity, &used, pcs);
    if (status != CP_OK) return status;
    status = cp_report_append(buffer, capacity, &used, ",\n  \"description\": ");
    if (status != CP_OK) return status;
    if (profile->parsed.description.text) {
        status = cp_json_append_escaped(buffer, capacity, &used,
                                        profile->parsed.description.text);
    } else {
        status = cp_report_append(buffer, capacity, &used, "null");
    }
    if (status != CP_OK) return status;
    status = cp_report_append(buffer, capacity, &used, ",\n  \"tags\": [\n");
    if (status != CP_OK) return status;
    for (size_t i = 0; i < profile->parsed.tag_count; ++i) {
        char signature[5];
        char type[5];
        cp_signature_text(profile->parsed.tags[i].signature, signature);
        cp_signature_text(profile->parsed.tags[i].type_signature, type);
        status = cp_report_append(buffer, capacity, &used,
                                  "    {\"signature\": ");
        if (status != CP_OK) return status;
        status = cp_json_append_escaped(buffer, capacity, &used, signature);
        if (status != CP_OK) return status;
        status = cp_report_append(buffer, capacity, &used, ", \"type\": ");
        if (status != CP_OK) return status;
        status = cp_json_append_escaped(buffer, capacity, &used, type);
        if (status != CP_OK) return status;
        status = cp_report_append(buffer, capacity, &used,
            ", \"offset\": %u, \"size\": %u}%s\n",
            profile->parsed.tags[i].offset, profile->parsed.tags[i].size,
            i + 1 == profile->parsed.tag_count ? "" : ",");
        if (status != CP_OK) return status;
    }
    status = cp_report_append(buffer, capacity, &used, "  ]\n}\n");
    if (status != CP_OK) return status;
    if (written) *written = used;
    return CP_OK;
}

cp_status cp_profile_format_csv(const cp_profile *profile, char *buffer,
                                size_t capacity, size_t *written) {
    if (!profile || !buffer || capacity == 0) return CP_ERR_ARGUMENT;
    size_t used = 0;
    cp_status status = cp_report_append(buffer, capacity, &used,
        "index,signature,type,offset,size,name\n");
    if (status != CP_OK) return status;
    for (size_t i = 0; i < profile->parsed.tag_count; ++i) {
        char signature[5];
        char type[5];
        cp_signature_text(profile->parsed.tags[i].signature, signature);
        cp_signature_text(profile->parsed.tags[i].type_signature, type);
        status = cp_report_append(buffer, capacity, &used,
            "%zu,%s,%s,%u,%u,\"%s\"\n", i, signature, type,
            profile->parsed.tags[i].offset, profile->parsed.tags[i].size,
            cp_signature_name(profile->parsed.tags[i].signature));
        if (status != CP_OK) return status;
    }
    if (written) *written = used;
    return CP_OK;
}
