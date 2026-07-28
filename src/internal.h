#ifndef CHROMAPATH_INTERNAL_H
#define CHROMAPATH_INTERNAL_H

#include "chromapath.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct cp_reader {
    const uint8_t *data;
    size_t size;
    size_t pos;
} cp_reader;

typedef struct cp_tag_record {
    uint32_t signature;
    uint32_t offset;
    uint32_t size;
    uint32_t type_signature;
} cp_tag_record;

typedef struct cp_curve {
    uint32_t count;
    double gamma;
    double *values;
} cp_curve;

typedef struct cp_matrix3 {
    double v[3][3];
} cp_matrix3;

typedef struct cp_text_value {
    char *text;
    size_t length;
} cp_text_value;

typedef struct cp_curve_view {
    uint32_t count;
    double gamma;
    const double *values;
    uint32_t source_signature;
} cp_curve_view;

typedef struct cp_transform_plan {
    cp_curve_view channel[3];
    double channel_area[3];
    uint32_t generation;
} cp_transform_plan;

typedef struct cp_profile_data {
    cp_header_info header;
    cp_tag_record *tags;
    size_t tag_count;
    cp_text_value description;
    cp_xyz white_point;
    bool has_white_point;
    cp_curve red_curve;
    cp_curve green_curve;
    cp_curve blue_curve;
    cp_matrix3 rgb_to_xyz;
    bool has_matrix;
    uint8_t curve_route[3];
    cp_transform_plan default_plan;
} cp_profile_data;

struct cp_profile {
    uint8_t *storage;
    size_t storage_size;
    cp_parse_options options;
    cp_profile_data parsed;
    char error[192];
};

struct cp_transform {
    cp_curve red_curve;
    cp_curve green_curve;
    cp_curve blue_curve;
    cp_matrix3 matrix;
    cp_xyz white_point;
    uint32_t intent;
};

void cp_set_error(cp_profile *profile, const char *format, ...);
void cp_reader_init(cp_reader *reader, const uint8_t *data, size_t size);
bool cp_reader_seek(cp_reader *reader, size_t offset);
bool cp_reader_skip(cp_reader *reader, size_t amount);
bool cp_read_u8(cp_reader *reader, uint8_t *out);
bool cp_read_u16be(cp_reader *reader, uint16_t *out);
bool cp_read_u32be(cp_reader *reader, uint32_t *out);
bool cp_read_u64be(cp_reader *reader, uint64_t *out);
bool cp_read_s32be(cp_reader *reader, int32_t *out);
bool cp_read_bytes(cp_reader *reader, void *out, size_t amount);
bool cp_slice(const uint8_t *data, size_t size, size_t offset, size_t length,
              const uint8_t **out);
double cp_s15fixed16_to_double(int32_t value);

cp_status cp_parse_header(cp_profile *profile);
cp_status cp_parse_directory(cp_profile *profile);
cp_status cp_decode_tags(cp_profile *profile);
cp_status cp_validate_profile(cp_profile *profile);
cp_status cp_prepare_default_plan(cp_profile *profile);
cp_status cp_reconcile_profile_model(cp_profile *profile);
void cp_profile_data_destroy(cp_profile_data *data);

cp_status cp_decode_text_tag(cp_profile *profile, const cp_tag_record *tag,
                             cp_text_value *out);
cp_status cp_decode_xyz_tag(cp_profile *profile, const cp_tag_record *tag,
                            cp_xyz *out);
cp_status cp_decode_curve_tag(cp_profile *profile, const cp_tag_record *tag,
                              cp_curve *out);
cp_status cp_decode_matrix_tags(cp_profile *profile);
void cp_curve_destroy(cp_curve *curve);
cp_status cp_curve_copy(const cp_curve *source, cp_curve *destination);
double cp_curve_eval(const cp_curve *curve, double value);
cp_status cp_curve_copy_view(const cp_curve_view *source, cp_curve *destination);

const cp_tag_record *cp_find_record(const cp_profile *profile, uint32_t signature);
cp_status cp_report_append(char *buffer, size_t capacity, size_t *used,
                           const char *format, ...);

#endif
