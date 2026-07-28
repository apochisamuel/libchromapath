#include "internal.h"

#include <string.h>

void cp_reader_init(cp_reader *reader, const uint8_t *data, size_t size) {
    if (!reader) return;
    reader->data = data;
    reader->size = size;
    reader->pos = 0;
}

bool cp_reader_seek(cp_reader *reader, size_t offset) {
    if (!reader || offset > reader->size) return false;
    reader->pos = offset;
    return true;
}

bool cp_reader_skip(cp_reader *reader, size_t amount) {
    if (!reader || amount > reader->size - reader->pos) return false;
    reader->pos += amount;
    return true;
}

bool cp_read_u8(cp_reader *reader, uint8_t *out) {
    if (!reader || !out || reader->pos >= reader->size) return false;
    *out = reader->data[reader->pos++];
    return true;
}

bool cp_read_u16be(cp_reader *reader, uint16_t *out) {
    if (!reader || !out || reader->size - reader->pos < 2) return false;
    const uint8_t *p = reader->data + reader->pos;
    *out = (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
    reader->pos += 2;
    return true;
}

bool cp_read_u32be(cp_reader *reader, uint32_t *out) {
    if (!reader || !out || reader->size - reader->pos < 4) return false;
    const uint8_t *p = reader->data + reader->pos;
    *out = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    reader->pos += 4;
    return true;
}

bool cp_read_u64be(cp_reader *reader, uint64_t *out) {
    uint32_t hi, lo;
    if (!cp_read_u32be(reader, &hi) || !cp_read_u32be(reader, &lo)) return false;
    *out = ((uint64_t)hi << 32) | lo;
    return true;
}

bool cp_read_s32be(cp_reader *reader, int32_t *out) {
    uint32_t value;
    if (!cp_read_u32be(reader, &value)) return false;
    *out = (int32_t)value;
    return true;
}

bool cp_read_bytes(cp_reader *reader, void *out, size_t amount) {
    if (!reader || (!out && amount) || amount > reader->size - reader->pos) return false;
    if (amount) memcpy(out, reader->data + reader->pos, amount);
    reader->pos += amount;
    return true;
}

bool cp_slice(const uint8_t *data, size_t size, size_t offset, size_t length,
              const uint8_t **out) {
    if (!data || !out || offset > size || length > size - offset) return false;
    *out = data + offset;
    return true;
}

double cp_s15fixed16_to_double(int32_t value) {
    return (double)value / 65536.0;
}
