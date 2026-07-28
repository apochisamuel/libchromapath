#include "internal.h"

#include <math.h>

static bool finite_xyz(cp_xyz value) {
    return isfinite(value.x) && isfinite(value.y) && isfinite(value.z);
}

static bool matrix_finite(const cp_matrix3 *m) {
    for (size_t row = 0; row < 3; ++row) {
        for (size_t col = 0; col < 3; ++col) {
            if (!isfinite(m->v[row][col])) return false;
        }
    }
    return true;
}

cp_status cp_validate_profile(cp_profile *profile) {
    if (!profile) return CP_ERR_ARGUMENT;
    for (size_t i = 0; i < profile->parsed.tag_count; ++i) {
        const cp_tag_record *a = &profile->parsed.tags[i];
        for (size_t j = i + 1; j < profile->parsed.tag_count; ++j) {
            const cp_tag_record *b = &profile->parsed.tags[j];
            if (a->signature == b->signature && profile->options.strict) {
                cp_set_error(profile, "duplicate tag signature");
                return CP_ERR_FORMAT;
            }
        }
    }
    if (profile->parsed.has_white_point && !finite_xyz(profile->parsed.white_point)) {
        cp_set_error(profile, "media white point is not finite");
        return CP_ERR_FORMAT;
    }
    if (!matrix_finite(&profile->parsed.rgb_to_xyz)) {
        cp_set_error(profile, "profile matrix is not finite");
        return CP_ERR_FORMAT;
    }
    return cp_reconcile_profile_model(profile);
}
