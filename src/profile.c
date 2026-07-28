#include "internal.h"

#include <stdlib.h>
#include <string.h>

void cp_profile_data_destroy(cp_profile_data *data) {
    if (!data) return;
    free(data->tags);
    free(data->description.text);
    cp_curve_destroy(&data->red_curve);
    cp_curve_destroy(&data->green_curve);
    cp_curve_destroy(&data->blue_curve);
    memset(data, 0, sizeof(*data));
}

cp_status cp_profile_open_memory(const uint8_t *data, size_t size,
                                 const cp_parse_options *options,
                                 cp_profile **out_profile) {
    if (!data || !out_profile || size < 132) return CP_ERR_ARGUMENT;
    *out_profile = NULL;
    cp_profile *profile = (cp_profile *)calloc(1, sizeof(*profile));
    if (!profile) return CP_ERR_NOMEM;
    cp_parse_options_init(&profile->options);
    if (options) profile->options = *options;
    if (profile->options.max_profile_size == 0 || size > profile->options.max_profile_size) {
        free(profile);
        return CP_ERR_FORMAT;
    }
    profile->storage = (uint8_t *)malloc(size);
    if (!profile->storage) {
        free(profile);
        return CP_ERR_NOMEM;
    }
    memcpy(profile->storage, data, size);
    profile->storage_size = size;
    cp_status st = cp_parse_header(profile);
    if (st == CP_OK) st = cp_parse_directory(profile);
    if (st == CP_OK) st = cp_decode_tags(profile);
    if (st == CP_OK) st = cp_prepare_default_plan(profile);
    if (st == CP_OK) st = cp_validate_profile(profile);
    if (st != CP_OK) {
        cp_profile_close(profile);
        return st;
    }
    *out_profile = profile;
    return CP_OK;
}

void cp_profile_close(cp_profile *profile) {
    if (!profile) return;
    cp_profile_data_destroy(&profile->parsed);
    free(profile->storage);
    free(profile);
}

cp_status cp_profile_header(const cp_profile *profile, cp_header_info *out) {
    if (!profile || !out) return CP_ERR_ARGUMENT;
    *out = profile->parsed.header;
    return CP_OK;
}

size_t cp_profile_tag_count(const cp_profile *profile) {
    return profile ? profile->parsed.tag_count : 0;
}

cp_status cp_profile_tag_at(const cp_profile *profile, size_t index, cp_tag_info *out) {
    if (!profile || !out) return CP_ERR_ARGUMENT;
    if (index >= profile->parsed.tag_count) return CP_ERR_NOT_FOUND;
    const cp_tag_record *tag = &profile->parsed.tags[index];
    out->signature = tag->signature;
    out->type_signature = tag->type_signature;
    out->offset = tag->offset;
    out->size = tag->size;
    return CP_OK;
}

cp_status cp_profile_find_tag(const cp_profile *profile, uint32_t signature, cp_tag_info *out) {
    if (!profile || !out) return CP_ERR_ARGUMENT;
    const cp_tag_record *tag = cp_find_record(profile, signature);
    if (!tag) return CP_ERR_NOT_FOUND;
    out->signature = tag->signature;
    out->type_signature = tag->type_signature;
    out->offset = tag->offset;
    out->size = tag->size;
    return CP_OK;
}

cp_status cp_profile_description(const cp_profile *profile, char *buffer, size_t capacity) {
    if (!profile || !buffer || capacity == 0) return CP_ERR_ARGUMENT;
    if (!profile->parsed.description.text) return CP_ERR_NOT_FOUND;
    if (profile->parsed.description.length + 1 > capacity) return CP_ERR_ARGUMENT;
    memcpy(buffer, profile->parsed.description.text, profile->parsed.description.length + 1);
    return CP_OK;
}

cp_status cp_profile_media_white_point(const cp_profile *profile, cp_xyz *out) {
    if (!profile || !out) return CP_ERR_ARGUMENT;
    if (!profile->parsed.has_white_point) return CP_ERR_NOT_FOUND;
    *out = profile->parsed.white_point;
    return CP_OK;
}
