#include "internal.h"

#include <math.h>
#include <string.h>

static int cp_header_equal(const cp_header_info *left,
                           const cp_header_info *right) {
    return left->declared_size == right->declared_size &&
           left->preferred_cmm == right->preferred_cmm &&
           left->version == right->version &&
           left->device_class == right->device_class &&
           left->color_space == right->color_space &&
           left->pcs == right->pcs &&
           left->platform == right->platform &&
           left->flags == right->flags &&
           left->manufacturer == right->manufacturer &&
           left->model == right->model &&
           left->attributes == right->attributes &&
           left->rendering_intent == right->rendering_intent &&
           left->creator == right->creator;
}

static int cp_payload_equal(const cp_profile *left, const cp_tag_record *left_tag,
                            const cp_profile *right, const cp_tag_record *right_tag) {
    if (left_tag->size != right_tag->size) return 0;
    const uint8_t *left_data;
    const uint8_t *right_data;
    if (!cp_slice(left->storage, left->storage_size, left_tag->offset,
                  left_tag->size, &left_data)) return 0;
    if (!cp_slice(right->storage, right->storage_size, right_tag->offset,
                  right_tag->size, &right_data)) return 0;
    return memcmp(left_data, right_data, left_tag->size) == 0;
}

cp_status cp_profile_compare(const cp_profile *left, const cp_profile *right,
                             cp_profile_difference *difference) {
    if (!left || !right || !difference) return CP_ERR_ARGUMENT;
    memset(difference, 0, sizeof(*difference));
    difference->header_equal = cp_header_equal(&left->parsed.header,
                                               &right->parsed.header);
    for (size_t i = 0; i < left->parsed.tag_count; ++i) {
        const cp_tag_record *left_tag = &left->parsed.tags[i];
        const cp_tag_record *right_tag = cp_find_record(right, left_tag->signature);
        if (!right_tag) {
            difference->missing_from_right += 1;
            continue;
        }
        difference->matching_tags += 1;
        if (!cp_payload_equal(left, left_tag, right, right_tag)) {
            difference->differing_payloads += 1;
        }
    }
    for (size_t i = 0; i < right->parsed.tag_count; ++i) {
        if (!cp_find_record(left, right->parsed.tags[i].signature)) {
            difference->missing_from_left += 1;
        }
    }
    if (left->parsed.has_white_point && right->parsed.has_white_point) {
        double dx = left->parsed.white_point.x - right->parsed.white_point.x;
        double dy = left->parsed.white_point.y - right->parsed.white_point.y;
        double dz = left->parsed.white_point.z - right->parsed.white_point.z;
        difference->white_point_distance = sqrt(dx * dx + dy * dy + dz * dz);
    }
    return CP_OK;
}
