#include "internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct cp_tag_rule {
    uint32_t signature;
    uint32_t allowed_type_1;
    uint32_t allowed_type_2;
    int required_for_rgb;
    const char *label;
} cp_tag_rule;

#define SIG(a,b,c,d) ((uint32_t)(a)<<24 | (uint32_t)(b)<<16 | (uint32_t)(c)<<8 | (uint32_t)(d))

static const cp_tag_rule cp_tag_rules[] = {
    {SIG('d','e','s','c'), SIG('d','e','s','c'), SIG('m','l','u','c'), 0, "profile description"},
    {SIG('c','p','r','t'), SIG('t','e','x','t'), SIG('m','l','u','c'), 0, "copyright"},
    {SIG('w','t','p','t'), SIG('X','Y','Z',' '), 0, 1, "media white point"},
    {SIG('b','k','p','t'), SIG('X','Y','Z',' '), 0, 0, "media black point"},
    {SIG('r','X','Y','Z'), SIG('X','Y','Z',' '), 0, 1, "red colorant"},
    {SIG('g','X','Y','Z'), SIG('X','Y','Z',' '), 0, 1, "green colorant"},
    {SIG('b','X','Y','Z'), SIG('X','Y','Z',' '), 0, 1, "blue colorant"},
    {SIG('r','T','R','C'), SIG('c','u','r','v'), SIG('p','a','r','a'), 1, "red response"},
    {SIG('g','T','R','C'), SIG('c','u','r','v'), SIG('p','a','r','a'), 1, "green response"},
    {SIG('b','T','R','C'), SIG('c','u','r','v'), SIG('p','a','r','a'), 1, "blue response"},
    {SIG('k','T','R','C'), SIG('c','u','r','v'), SIG('p','a','r','a'), 0, "gray response"},
    {SIG('c','h','a','d'), SIG('s','f','3','2'), 0, 0, "adaptation matrix"},
    {SIG('c','h','r','m'), SIG('c','h','r','m'), 0, 0, "chromaticity"},
    {SIG('l','u','m','i'), SIG('X','Y','Z',' '), 0, 0, "luminance"},
    {SIG('m','e','a','s'), SIG('m','e','a','s'), 0, 0, "measurement"},
    {SIG('v','i','e','w'), SIG('v','i','e','w'), 0, 0, "viewing conditions"},
    {SIG('v','u','e','d'), SIG('d','e','s','c'), SIG('m','l','u','c'), 0, "viewing description"},
    {SIG('d','m','n','d'), SIG('d','e','s','c'), SIG('m','l','u','c'), 0, "manufacturer description"},
    {SIG('d','m','d','d'), SIG('d','e','s','c'), SIG('m','l','u','c'), 0, "model description"},
    {SIG('t','e','c','h'), SIG('s','i','g',' '), 0, 0, "device technology"},
    {SIG('A','2','B','0'), SIG('m','f','t','1'), SIG('m','f','t','2'), 0, "perceptual input transform"},
    {SIG('A','2','B','1'), SIG('m','f','t','1'), SIG('m','f','t','2'), 0, "colorimetric input transform"},
    {SIG('A','2','B','2'), SIG('m','f','t','1'), SIG('m','f','t','2'), 0, "saturation input transform"},
    {SIG('B','2','A','0'), SIG('m','f','t','1'), SIG('m','f','t','2'), 0, "perceptual output transform"},
    {SIG('B','2','A','1'), SIG('m','f','t','1'), SIG('m','f','t','2'), 0, "colorimetric output transform"},
    {SIG('B','2','A','2'), SIG('m','f','t','1'), SIG('m','f','t','2'), 0, "saturation output transform"},
    {SIG('g','a','m','t'), SIG('m','f','t','1'), SIG('m','f','t','2'), 0, "gamut transform"},
    {SIG('n','c','l','2'), SIG('n','c','l','2'), 0, 0, "named color table"},
    {SIG('p','s','e','q'), SIG('p','s','e','q'), 0, 0, "profile sequence"},
    {SIG('p','s','i','d'), SIG('p','s','i','d'), 0, 0, "profile sequence identifiers"},
    {SIG('r','e','s','p'), SIG('r','c','s','2'), 0, 0, "response curve set"},
    {SIG('m','e','t','a'), SIG('d','i','c','t'), 0, 0, "metadata dictionary"},
    {SIG('c','a','l','t'), SIG('d','t','i','m'), 0, 0, "calibration timestamp"},
    {SIG('c','i','c','p'), SIG('u','i','0','8'), 0, 0, "video code points"},
    {SIG('c','e','p','t'), SIG('c','e','p','t'), 0, 0, "encoding parameters"},
    {SIG('a','r','t','s'), SIG('m','f','t','2'), SIG('m','p','e','t'), 0, "reference gamut"}
};

static void cp_emit_diagnostic(cp_diagnostic *items, size_t capacity,
                               size_t *count, int severity, uint32_t signature,
                               const char *format, ...) {
    size_t index = *count;
    *count += 1;
    if (!items || index >= capacity) return;
    items[index].severity = severity;
    items[index].tag_signature = signature;
    va_list args;
    va_start(args, format);
    vsnprintf(items[index].message, sizeof(items[index].message), format, args);
    va_end(args);
}

static const cp_tag_rule *cp_rule_for(uint32_t signature) {
    for (size_t i = 0; i < sizeof(cp_tag_rules) / sizeof(cp_tag_rules[0]); ++i) {
        if (cp_tag_rules[i].signature == signature) return &cp_tag_rules[i];
    }
    return NULL;
}

size_t cp_profile_diagnostics(const cp_profile *profile, cp_diagnostic *items,
                              size_t capacity) {
    if (!profile) return 0;
    size_t count = 0;
    if (profile->parsed.header.declared_size != profile->storage_size) {
        cp_emit_diagnostic(items, capacity, &count, 2, 0,
                           "Declared size differs from available profile bytes");
    }
    if (!cp_color_space_channels(profile->parsed.header.color_space)) {
        cp_emit_diagnostic(items, capacity, &count, 2, 0,
                           "Device color space is not recognized");
    }
    if (!cp_color_space_is_pcs(profile->parsed.header.pcs)) {
        cp_emit_diagnostic(items, capacity, &count, 2, 0,
                           "Profile connection space is not XYZ or Lab");
    }
    for (size_t i = 0; i < profile->parsed.tag_count; ++i) {
        const cp_tag_record *tag = &profile->parsed.tags[i];
        const cp_tag_rule *rule = cp_rule_for(tag->signature);
        if (rule && tag->type_signature != rule->allowed_type_1 &&
            tag->type_signature != rule->allowed_type_2) {
            cp_emit_diagnostic(items, capacity, &count, 2, tag->signature,
                               "%s uses an unexpected payload type", rule->label);
        }
        if (tag->offset % 4 != 0) {
            cp_emit_diagnostic(items, capacity, &count, 1, tag->signature,
                               "Tag payload offset is not four-byte aligned");
        }
        if (tag->size < 8) {
            cp_emit_diagnostic(items, capacity, &count, 2, tag->signature,
                               "Tag payload is shorter than a type header");
        }
        for (size_t j = i + 1; j < profile->parsed.tag_count; ++j) {
            const cp_tag_record *other = &profile->parsed.tags[j];
            if (tag->signature == other->signature) {
                cp_emit_diagnostic(items, capacity, &count, 1, tag->signature,
                                   "Tag directory contains a repeated signature");
            }
            uint64_t left_end = (uint64_t)tag->offset + tag->size;
            uint64_t right_end = (uint64_t)other->offset + other->size;
            int overlap = (uint64_t)tag->offset < right_end &&
                          (uint64_t)other->offset < left_end;
            int identical = tag->offset == other->offset && tag->size == other->size;
            if (overlap && !identical) {
                cp_emit_diagnostic(items, capacity, &count, 2, tag->signature,
                                   "Tag payload partially overlaps another payload");
            }
        }
    }
    if (profile->parsed.header.color_space == SIG('R','G','B',' ')) {
        for (size_t i = 0; i < sizeof(cp_tag_rules) / sizeof(cp_tag_rules[0]); ++i) {
            if (cp_tag_rules[i].required_for_rgb &&
                !cp_find_record(profile, cp_tag_rules[i].signature)) {
                cp_emit_diagnostic(items, capacity, &count, 2,
                                   cp_tag_rules[i].signature,
                                   "RGB profile omits required %s", cp_tag_rules[i].label);
            }
        }
    }
    if (profile->parsed.has_white_point) {
        if (!(profile->parsed.white_point.x > 0.0) ||
            !(profile->parsed.white_point.y > 0.0) ||
            !(profile->parsed.white_point.z > 0.0)) {
            cp_emit_diagnostic(items, capacity, &count, 2, SIG('w','t','p','t'),
                               "Media white point must contain positive components");
        }
    }
    return count;
}
