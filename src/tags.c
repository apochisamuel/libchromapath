#include "internal.h"

#include <stdlib.h>

static cp_status decode_optional_description(cp_profile *profile) {
    const cp_tag_record *tag = cp_find_record(profile, cp_signature("desc"));
    if (!tag) return CP_OK;
    cp_status st = cp_decode_text_tag(profile, tag, &profile->parsed.description);
    if (st == CP_ERR_UNSUPPORTED && !profile->options.strict) return CP_OK;
    return st;
}

static cp_status decode_optional_white_point(cp_profile *profile) {
    const cp_tag_record *tag = cp_find_record(profile, cp_signature("wtpt"));
    if (!tag) return CP_OK;
    cp_status st = cp_decode_xyz_tag(profile, tag, &profile->parsed.white_point);
    if (st == CP_OK) profile->parsed.has_white_point = true;
    return st;
}

static cp_status decode_curve(cp_profile *profile, const char sig[4], cp_curve *curve) {
    const cp_tag_record *tag = cp_find_record(profile, cp_signature(sig));
    if (!tag) return CP_OK;
    cp_status st = cp_decode_curve_tag(profile, tag, curve);
    if (st == CP_ERR_UNSUPPORTED && !profile->options.strict) return CP_OK;
    return st;
}

cp_status cp_decode_tags(cp_profile *profile) {
    cp_status st = decode_optional_description(profile);
    if (st != CP_OK) return st;
    st = decode_optional_white_point(profile);
    if (st != CP_OK && profile->options.strict) return st;
    st = decode_curve(profile, "rTRC", &profile->parsed.red_curve);
    if (st != CP_OK) return st;
    st = decode_curve(profile, "gTRC", &profile->parsed.green_curve);
    if (st != CP_OK) return st;
    st = decode_curve(profile, "bTRC", &profile->parsed.blue_curve);
    if (st != CP_OK) return st;
    return cp_decode_matrix_tags(profile);
}
