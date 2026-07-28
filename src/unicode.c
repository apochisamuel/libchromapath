#include "chromapath.h"

#include <stddef.h>
#include <stdint.h>

static cp_status cp_utf8_decode_one(const uint8_t *text, size_t length,
                                    size_t *consumed, uint32_t *codepoint) {
    if (!text || !consumed || !codepoint || length == 0) return CP_ERR_ARGUMENT;
    uint8_t first = text[0];
    if (first < 0x80) {
        *consumed = 1;
        *codepoint = first;
        return CP_OK;
    }
    size_t need;
    uint32_t value;
    uint32_t minimum;
    if ((first & 0xe0u) == 0xc0u) {
        need = 2;
        value = first & 0x1fu;
        minimum = 0x80u;
    } else if ((first & 0xf0u) == 0xe0u) {
        need = 3;
        value = first & 0x0fu;
        minimum = 0x800u;
    } else if ((first & 0xf8u) == 0xf0u) {
        need = 4;
        value = first & 0x07u;
        minimum = 0x10000u;
    } else {
        return CP_ERR_FORMAT;
    }
    if (need > length) return CP_ERR_TRUNCATED;
    for (size_t i = 1; i < need; ++i) {
        if ((text[i] & 0xc0u) != 0x80u) return CP_ERR_FORMAT;
        value = (value << 6) | (text[i] & 0x3fu);
    }
    if (value < minimum || value > 0x10ffffu ||
        (value >= 0xd800u && value <= 0xdfffu)) return CP_ERR_FORMAT;
    *consumed = need;
    *codepoint = value;
    return CP_OK;
}

static size_t cp_utf8_encoded_size(uint32_t codepoint) {
    if (codepoint <= 0x7fu) return 1;
    if (codepoint <= 0x7ffu) return 2;
    if (codepoint <= 0xffffu) return 3;
    if (codepoint <= 0x10ffffu) return 4;
    return 0;
}

static cp_status cp_utf8_encode_one(uint32_t codepoint, char *destination,
                                    size_t capacity, size_t *written) {
    size_t size = cp_utf8_encoded_size(codepoint);
    if (!size || !destination || capacity < size || !written) return CP_ERR_OVERFLOW;
    if (size == 1) {
        destination[0] = (char)codepoint;
    } else if (size == 2) {
        destination[0] = (char)(0xc0u | (codepoint >> 6));
        destination[1] = (char)(0x80u | (codepoint & 0x3fu));
    } else if (size == 3) {
        destination[0] = (char)(0xe0u | (codepoint >> 12));
        destination[1] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
        destination[2] = (char)(0x80u | (codepoint & 0x3fu));
    } else {
        destination[0] = (char)(0xf0u | (codepoint >> 18));
        destination[1] = (char)(0x80u | ((codepoint >> 12) & 0x3fu));
        destination[2] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
        destination[3] = (char)(0x80u | (codepoint & 0x3fu));
    }
    *written = size;
    return CP_OK;
}

cp_status cp_utf8_validate(const char *text, size_t length, size_t *codepoints) {
    if (!text && length) return CP_ERR_ARGUMENT;
    size_t position = 0;
    size_t count = 0;
    while (position < length) {
        size_t consumed;
        uint32_t codepoint;
        cp_status status = cp_utf8_decode_one((const uint8_t *)text + position,
                                              length - position, &consumed,
                                              &codepoint);
        if (status != CP_OK) return status;
        position += consumed;
        count += 1;
    }
    if (codepoints) *codepoints = count;
    return CP_OK;
}

cp_status cp_utf16be_to_utf8(const uint8_t *source, size_t source_size,
                             char *destination, size_t capacity, size_t *written) {
    if (!source || !destination || source_size % 2 != 0) return CP_ERR_ARGUMENT;
    size_t source_position = 0;
    size_t destination_position = 0;
    while (source_position < source_size) {
        uint32_t first = ((uint32_t)source[source_position] << 8) |
                         source[source_position + 1];
        source_position += 2;
        uint32_t codepoint = first;
        if (first >= 0xd800u && first <= 0xdbffu) {
            if (source_position + 2 > source_size) return CP_ERR_TRUNCATED;
            uint32_t second = ((uint32_t)source[source_position] << 8) |
                              source[source_position + 1];
            source_position += 2;
            if (second < 0xdc00u || second > 0xdfffu) return CP_ERR_FORMAT;
            codepoint = 0x10000u + ((first - 0xd800u) << 10) +
                        (second - 0xdc00u);
        } else if (first >= 0xdc00u && first <= 0xdfffu) {
            return CP_ERR_FORMAT;
        }
        size_t encoded;
        cp_status status = cp_utf8_encode_one(codepoint,
                                              destination + destination_position,
                                              capacity - destination_position,
                                              &encoded);
        if (status != CP_OK) return status;
        destination_position += encoded;
    }
    if (destination_position >= capacity) return CP_ERR_OVERFLOW;
    destination[destination_position] = '\0';
    if (written) *written = destination_position;
    return CP_OK;
}

cp_status cp_utf8_normalize_line_endings(const char *source, size_t source_size,
                                         char *destination, size_t capacity,
                                         size_t *written) {
    if (!source || !destination || capacity == 0) return CP_ERR_ARGUMENT;
    cp_status status = cp_utf8_validate(source, source_size, NULL);
    if (status != CP_OK) return status;
    size_t input = 0;
    size_t output = 0;
    while (input < source_size) {
        char value = source[input++];
        if (value == '\r') {
            if (input < source_size && source[input] == '\n') input += 1;
            value = '\n';
        }
        if (output + 1 >= capacity) return CP_ERR_OVERFLOW;
        destination[output++] = value;
    }
    destination[output] = '\0';
    if (written) *written = output;
    return CP_OK;
}
