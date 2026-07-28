#include "internal.h"

#include <stdlib.h>
#include <string.h>

static cp_status decode_desc(cp_profile *profile, cp_reader *reader,
                             cp_text_value *out) {
    uint32_t ascii_count;
    if (!cp_read_u32be(reader, &ascii_count)) return CP_ERR_TRUNCATED;
    if (ascii_count == 0 || ascii_count > profile->options.max_text_bytes) return CP_ERR_FORMAT;
    if (ascii_count > reader->size - reader->pos) return CP_ERR_TRUNCATED;
    char *text = (char *)malloc(ascii_count);
    if (!text) return CP_ERR_NOMEM;
    if (!cp_read_bytes(reader, text, ascii_count)) {
        free(text);
        return CP_ERR_TRUNCATED;
    }
    size_t length = ascii_count;
    if (length && text[length - 1] == '\0') --length;
    text[length] = '\0';
    out->text = text;
    out->length = length;
    return CP_OK;
}

static cp_status decode_text(cp_profile *profile, cp_reader *reader,
                             cp_text_value *out) {
    size_t remain = reader->size - reader->pos;
    if (remain > profile->options.max_text_bytes) return CP_ERR_FORMAT;
    char *text = (char *)malloc(remain + 1);
    if (!text) return CP_ERR_NOMEM;
    if (!cp_read_bytes(reader, text, remain)) {
        free(text);
        return CP_ERR_TRUNCATED;
    }
    text[remain] = '\0';
    while (remain && text[remain - 1] == '\0') --remain;
    text[remain] = '\0';
    out->text = text;
    out->length = remain;
    return CP_OK;
}

cp_status cp_decode_text_tag(cp_profile *profile, const cp_tag_record *tag,
                             cp_text_value *out) {
    if (!profile || !tag || !out) return CP_ERR_ARGUMENT;
    memset(out, 0, sizeof(*out));
    cp_reader reader;
    cp_reader_init(&reader, profile->storage + tag->offset, tag->size);
    uint32_t type, reserved;
    if (!cp_read_u32be(&reader, &type) || !cp_read_u32be(&reader, &reserved)) {
        return CP_ERR_TRUNCATED;
    }
    (void)reserved;
    if (type == cp_signature("desc")) return decode_desc(profile, &reader, out);
    if (type == cp_signature("text")) return decode_text(profile, &reader, out);
    return CP_ERR_UNSUPPORTED;
}
