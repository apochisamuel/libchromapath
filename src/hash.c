#include "chromapath.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint32_t cp_rotate_left(uint32_t value, uint32_t amount) {
    return (value << amount) | (value >> (32u - amount));
}

static uint32_t cp_load_le32(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static void cp_store_le32(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static void cp_md5_block(uint32_t state[4], const uint8_t block[64]) {
    static const uint32_t shifts[64] = {
        7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
        5,9,14,20, 5,9,14,20, 5,9,14,20, 5,9,14,20,
        4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
        6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
    };
    static const uint32_t constants[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
        0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,
        0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,
        0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,
        0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,
        0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
        0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,
        0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,
        0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
    };
    uint32_t words[16];
    for (size_t i = 0; i < 16; ++i) words[i] = cp_load_le32(block + i * 4);
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    for (uint32_t i = 0; i < 64; ++i) {
        uint32_t f;
        uint32_t g;
        if (i < 16) {
            f = (b & c) | ((~b) & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | ((~d) & c);
            g = (5u * i + 1u) % 16u;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3u * i + 5u) % 16u;
        } else {
            f = c ^ (b | (~d));
            g = (7u * i) % 16u;
        }
        uint32_t next = d;
        d = c;
        c = b;
        b = b + cp_rotate_left(a + f + constants[i] + words[g], shifts[i]);
        a = next;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void cp_md5(const uint8_t *data, size_t size, uint8_t digest[16]) {
    if (!digest) return;
    uint32_t state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
    size_t full_blocks = size / 64;
    for (size_t i = 0; i < full_blocks; ++i) {
        cp_md5_block(state, data + i * 64);
    }
    uint8_t final_blocks[128];
    size_t remainder = size % 64;
    memset(final_blocks, 0, sizeof(final_blocks));
    if (data && remainder) memcpy(final_blocks, data + full_blocks * 64, remainder);
    final_blocks[remainder] = 0x80;
    size_t final_size = remainder < 56 ? 64 : 128;
    uint64_t bit_length = (uint64_t)size * 8u;
    for (size_t i = 0; i < 8; ++i) {
        final_blocks[final_size - 8 + i] = (uint8_t)(bit_length >> (8u * i));
    }
    cp_md5_block(state, final_blocks);
    if (final_size == 128) cp_md5_block(state, final_blocks + 64);
    for (size_t i = 0; i < 4; ++i) cp_store_le32(digest + i * 4, state[i]);
}

uint32_t cp_crc32(const uint8_t *data, size_t size) {
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xedb88320u & mask);
        }
    }
    return ~crc;
}

uint64_t cp_fnv1a64(const uint8_t *data, size_t size) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}
