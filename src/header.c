#include "internal.h"

cp_status cp_parse_header(cp_profile *profile) {
    if (!profile || !profile->storage || profile->storage_size < 128) {
        return CP_ERR_ARGUMENT;
    }
    cp_reader reader;
    cp_reader_init(&reader, profile->storage, profile->storage_size);
    cp_header_info *h = &profile->parsed.header;
    uint32_t magic = 0;
    if (!cp_read_u32be(&reader, &h->declared_size) ||
        !cp_read_u32be(&reader, &h->preferred_cmm) ||
        !cp_read_u32be(&reader, &h->version) ||
        !cp_read_u32be(&reader, &h->device_class) ||
        !cp_read_u32be(&reader, &h->color_space) ||
        !cp_read_u32be(&reader, &h->pcs)) {
        cp_set_error(profile, "profile header is truncated");
        return CP_ERR_TRUNCATED;
    }
    if (!cp_reader_skip(&reader, 12) ||
        !cp_read_u32be(&reader, &magic) ||
        !cp_read_u32be(&reader, &h->platform) ||
        !cp_read_u32be(&reader, &h->flags) ||
        !cp_read_u32be(&reader, &h->manufacturer) ||
        !cp_read_u32be(&reader, &h->model) ||
        !cp_read_u64be(&reader, &h->attributes) ||
        !cp_read_u32be(&reader, &h->rendering_intent)) {
        cp_set_error(profile, "profile header is truncated");
        return CP_ERR_TRUNCATED;
    }
    int32_t x, y, z;
    if (!cp_read_s32be(&reader, &x) || !cp_read_s32be(&reader, &y) ||
        !cp_read_s32be(&reader, &z) || !cp_read_u32be(&reader, &h->creator)) {
        cp_set_error(profile, "profile header is truncated");
        return CP_ERR_TRUNCATED;
    }
    (void)x; (void)y; (void)z;
    if (magic != cp_signature("acsp")) {
        cp_set_error(profile, "profile signature is not acsp");
        return CP_ERR_FORMAT;
    }
    if (h->declared_size < 132 || h->declared_size > profile->storage_size) {
        cp_set_error(profile, "declared profile size is inconsistent");
        return CP_ERR_FORMAT;
    }
    if (h->declared_size > profile->options.max_profile_size) {
        cp_set_error(profile, "profile exceeds configured size limit");
        return CP_ERR_FORMAT;
    }
    return CP_OK;
}
