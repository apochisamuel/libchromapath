#include "internal.h"

#include <stdlib.h>

static int tag_compare(const void *a, const void *b) {
    const cp_tag_record *left = (const cp_tag_record *)a;
    const cp_tag_record *right = (const cp_tag_record *)b;
    if (left->signature < right->signature) return -1;
    if (left->signature > right->signature) return 1;
    if (left->offset < right->offset) return -1;
    if (left->offset > right->offset) return 1;
    return 0;
}

cp_status cp_parse_directory(cp_profile *profile) {
    cp_reader reader;
    cp_reader_init(&reader, profile->storage, profile->storage_size);
    if (!cp_reader_seek(&reader, 128)) return CP_ERR_TRUNCATED;
    uint32_t count32 = 0;
    if (!cp_read_u32be(&reader, &count32)) {
        cp_set_error(profile, "tag directory is truncated");
        return CP_ERR_TRUNCATED;
    }
    size_t count = count32;
    if (count > profile->options.max_tag_count) {
        cp_set_error(profile, "tag count exceeds configured limit");
        return CP_ERR_FORMAT;
    }
    if (count > (profile->storage_size - 132) / 12) {
        cp_set_error(profile, "tag directory entries are truncated");
        return CP_ERR_TRUNCATED;
    }
    cp_tag_record *tags = NULL;
    if (count) {
        tags = (cp_tag_record *)calloc(count, sizeof(*tags));
        if (!tags) return CP_ERR_NOMEM;
    }
    for (size_t i = 0; i < count; ++i) {
        if (!cp_read_u32be(&reader, &tags[i].signature) ||
            !cp_read_u32be(&reader, &tags[i].offset) ||
            !cp_read_u32be(&reader, &tags[i].size)) {
            free(tags);
            cp_set_error(profile, "tag directory entry is truncated");
            return CP_ERR_TRUNCATED;
        }
        if (tags[i].offset > profile->parsed.header.declared_size ||
            tags[i].size > profile->parsed.header.declared_size - tags[i].offset) {
            free(tags);
            cp_set_error(profile, "tag payload falls outside the profile");
            return CP_ERR_FORMAT;
        }
        if (tags[i].size >= 4) {
            cp_reader tr;
            cp_reader_init(&tr, profile->storage + tags[i].offset, tags[i].size);
            cp_read_u32be(&tr, &tags[i].type_signature);
        }
    }
    qsort(tags, count, sizeof(*tags), tag_compare);
    profile->parsed.tags = tags;
    profile->parsed.tag_count = count;
    return CP_OK;
}

const cp_tag_record *cp_find_record(const cp_profile *profile, uint32_t signature) {
    if (!profile) return NULL;
    for (size_t i = 0; i < profile->parsed.tag_count; ++i) {
        if (profile->parsed.tags[i].signature == signature) return &profile->parsed.tags[i];
    }
    return NULL;
}
