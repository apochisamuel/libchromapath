#include "chromapath.h"

#include <stddef.h>

typedef struct cp_color_space_record {
    uint32_t signature;
    const char *name;
    size_t channels;
    int pcs;
    int device;
} cp_color_space_record;

#define SIG(a,b,c,d) ((uint32_t)(a)<<24 | (uint32_t)(b)<<16 | (uint32_t)(c)<<8 | (uint32_t)(d))

static const cp_color_space_record cp_color_spaces[] = {
    {SIG('X','Y','Z',' '), "CIE XYZ", 3, 1, 0},
    {SIG('L','a','b',' '), "CIE Lab", 3, 1, 0},
    {SIG('L','u','v',' '), "CIE Luv", 3, 0, 0},
    {SIG('Y','C','b','r'), "YCbCr", 3, 0, 1},
    {SIG('Y','x','y',' '), "CIE Yxy", 3, 0, 0},
    {SIG('R','G','B',' '), "RGB", 3, 0, 1},
    {SIG('G','R','A','Y'), "Gray", 1, 0, 1},
    {SIG('H','S','V',' '), "HSV", 3, 0, 1},
    {SIG('H','L','S',' '), "HLS", 3, 0, 1},
    {SIG('C','M','Y','K'), "CMYK", 4, 0, 1},
    {SIG('C','M','Y',' '), "CMY", 3, 0, 1},
    {SIG('2','C','L','R'), "Two color", 2, 0, 1},
    {SIG('3','C','L','R'), "Three color", 3, 0, 1},
    {SIG('4','C','L','R'), "Four color", 4, 0, 1},
    {SIG('5','C','L','R'), "Five color", 5, 0, 1},
    {SIG('6','C','L','R'), "Six color", 6, 0, 1},
    {SIG('7','C','L','R'), "Seven color", 7, 0, 1},
    {SIG('8','C','L','R'), "Eight color", 8, 0, 1},
    {SIG('9','C','L','R'), "Nine color", 9, 0, 1},
    {SIG('A','C','L','R'), "Ten color", 10, 0, 1},
    {SIG('B','C','L','R'), "Eleven color", 11, 0, 1},
    {SIG('C','C','L','R'), "Twelve color", 12, 0, 1},
    {SIG('D','C','L','R'), "Thirteen color", 13, 0, 1},
    {SIG('E','C','L','R'), "Fourteen color", 14, 0, 1},
    {SIG('F','C','L','R'), "Fifteen color", 15, 0, 1}
};

static const cp_color_space_record *cp_lookup_color_space(uint32_t signature) {
    for (size_t i = 0; i < sizeof(cp_color_spaces) / sizeof(cp_color_spaces[0]); ++i) {
        if (cp_color_spaces[i].signature == signature) return &cp_color_spaces[i];
    }
    return NULL;
}

const char *cp_color_space_name(uint32_t signature) {
    const cp_color_space_record *record = cp_lookup_color_space(signature);
    return record ? record->name : "Unknown color space";
}

size_t cp_color_space_channels(uint32_t signature) {
    const cp_color_space_record *record = cp_lookup_color_space(signature);
    return record ? record->channels : 0;
}

int cp_color_space_is_pcs(uint32_t signature) {
    const cp_color_space_record *record = cp_lookup_color_space(signature);
    return record ? record->pcs : 0;
}

int cp_color_space_is_device(uint32_t signature) {
    const cp_color_space_record *record = cp_lookup_color_space(signature);
    return record ? record->device : 0;
}
