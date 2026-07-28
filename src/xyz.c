#include "internal.h"

cp_status cp_decode_xyz_tag(cp_profile *profile, const cp_tag_record *tag,
                            cp_xyz *out) {
    if (!profile || !tag || !out) return CP_ERR_ARGUMENT;
    cp_reader reader;
    cp_reader_init(&reader, profile->storage + tag->offset, tag->size);
    uint32_t type, reserved;
    int32_t x, y, z;
    if (!cp_read_u32be(&reader, &type) || !cp_read_u32be(&reader, &reserved) ||
        !cp_read_s32be(&reader, &x) || !cp_read_s32be(&reader, &y) ||
        !cp_read_s32be(&reader, &z)) return CP_ERR_TRUNCATED;
    (void)reserved;
    if (type != cp_signature("XYZ ")) return CP_ERR_UNSUPPORTED;
    out->x = cp_s15fixed16_to_double(x);
    out->y = cp_s15fixed16_to_double(y);
    out->z = cp_s15fixed16_to_double(z);
    return CP_OK;
}
