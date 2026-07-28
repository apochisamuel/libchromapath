#include "internal.h"

#include <string.h>

static void matrix_identity(cp_matrix3 *m) {
    memset(m, 0, sizeof(*m));
    m->v[0][0] = 1.0;
    m->v[1][1] = 1.0;
    m->v[2][2] = 1.0;
}

cp_status cp_decode_matrix_tags(cp_profile *profile) {
    if (!profile) return CP_ERR_ARGUMENT;
    const uint32_t sigs[3] = {
        cp_signature("rXYZ"), cp_signature("gXYZ"), cp_signature("bXYZ")
    };
    cp_xyz cols[3];
    for (size_t i = 0; i < 3; ++i) {
        const cp_tag_record *tag = cp_find_record(profile, sigs[i]);
        if (!tag) {
            matrix_identity(&profile->parsed.rgb_to_xyz);
            profile->parsed.has_matrix = false;
            return CP_OK;
        }
        cp_status st = cp_decode_xyz_tag(profile, tag, &cols[i]);
        if (st != CP_OK) return st;
    }
    profile->parsed.rgb_to_xyz.v[0][0] = cols[0].x;
    profile->parsed.rgb_to_xyz.v[1][0] = cols[0].y;
    profile->parsed.rgb_to_xyz.v[2][0] = cols[0].z;
    profile->parsed.rgb_to_xyz.v[0][1] = cols[1].x;
    profile->parsed.rgb_to_xyz.v[1][1] = cols[1].y;
    profile->parsed.rgb_to_xyz.v[2][1] = cols[1].z;
    profile->parsed.rgb_to_xyz.v[0][2] = cols[2].x;
    profile->parsed.rgb_to_xyz.v[1][2] = cols[2].y;
    profile->parsed.rgb_to_xyz.v[2][2] = cols[2].z;
    profile->parsed.has_matrix = true;
    return CP_OK;
}
