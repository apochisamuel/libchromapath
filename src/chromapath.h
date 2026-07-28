#ifndef CHROMAPATH_H
#define CHROMAPATH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cp_profile cp_profile;
typedef struct cp_transform cp_transform;
typedef struct cp_pipeline cp_pipeline;
typedef struct cp_clut cp_clut;
typedef struct cp_tone_curve cp_tone_curve;

typedef enum cp_status {
    CP_OK = 0,
    CP_ERR_ARGUMENT = 1,
    CP_ERR_NOMEM = 2,
    CP_ERR_TRUNCATED = 3,
    CP_ERR_FORMAT = 4,
    CP_ERR_UNSUPPORTED = 5,
    CP_ERR_NOT_FOUND = 6,
    CP_ERR_STATE = 7,
    CP_ERR_OVERFLOW = 8,
    CP_ERR_IO = 9
} cp_status;

typedef enum cp_signature_group {
    CP_SIGNATURE_UNKNOWN = 0,
    CP_SIGNATURE_TAG = 1,
    CP_SIGNATURE_TYPE = 2,
    CP_SIGNATURE_CLASS = 3,
    CP_SIGNATURE_COLOR_SPACE = 4,
    CP_SIGNATURE_PLATFORM = 5,
    CP_SIGNATURE_TECHNOLOGY = 6,
    CP_SIGNATURE_ILLUMINANT = 7
} cp_signature_group;

typedef enum cp_adaptation_method {
    CP_ADAPT_BRADFORD = 0,
    CP_ADAPT_CAT02 = 1,
    CP_ADAPT_VON_KRIES = 2,
    CP_ADAPT_XYZ_SCALING = 3
} cp_adaptation_method;

typedef enum cp_interpolation_method {
    CP_INTERPOLATE_NEAREST = 0,
    CP_INTERPOLATE_LINEAR = 1,
    CP_INTERPOLATE_TETRAHEDRAL = 2
} cp_interpolation_method;

typedef struct cp_parse_options {
    size_t max_profile_size;
    size_t max_tag_count;
    size_t max_text_bytes;
    size_t max_lut_cells;
    int strict;
} cp_parse_options;

typedef struct cp_header_info {
    uint32_t declared_size;
    uint32_t preferred_cmm;
    uint32_t version;
    uint32_t device_class;
    uint32_t color_space;
    uint32_t pcs;
    uint32_t platform;
    uint32_t flags;
    uint32_t manufacturer;
    uint32_t model;
    uint64_t attributes;
    uint32_t rendering_intent;
    uint32_t creator;
} cp_header_info;

typedef struct cp_tag_info {
    uint32_t signature;
    uint32_t type_signature;
    uint32_t offset;
    uint32_t size;
} cp_tag_info;

typedef struct cp_xyz {
    double x;
    double y;
    double z;
} cp_xyz;

typedef struct cp_xyy {
    double x;
    double y;
    double Y;
} cp_xyy;

typedef struct cp_lab {
    double L;
    double a;
    double b;
} cp_lab;

typedef struct cp_lch {
    double L;
    double C;
    double h;
} cp_lch;

typedef struct cp_rgb {
    double r;
    double g;
    double b;
} cp_rgb;

typedef struct cp_hsl {
    double h;
    double s;
    double l;
} cp_hsl;

typedef struct cp_hsv {
    double h;
    double s;
    double v;
} cp_hsv;

typedef struct cp_cmyk {
    double c;
    double m;
    double y;
    double k;
} cp_cmyk;

typedef struct cp_matrix3x3 {
    double v[3][3];
} cp_matrix3x3;

typedef struct cp_datetime {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
} cp_datetime;

typedef struct cp_signature_info {
    uint32_t signature;
    cp_signature_group group;
    const char *name;
    const char *description;
} cp_signature_info;

typedef struct cp_statistics {
    size_t count;
    double minimum;
    double maximum;
    double mean;
    double variance;
    double standard_deviation;
    double sum;
} cp_statistics;

typedef struct cp_statistics_accumulator {
    size_t count;
    double minimum;
    double maximum;
    double mean;
    double m2;
    double sum;
} cp_statistics_accumulator;

typedef struct cp_spectral_sample {
    double wavelength_nm;
    double value;
} cp_spectral_sample;

typedef struct cp_spectral_curve {
    const cp_spectral_sample *samples;
    size_t count;
} cp_spectral_curve;

typedef struct cp_reference_color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    cp_lab lab;
} cp_reference_color;

typedef struct cp_illuminant_info {
    const char *name;
    cp_xyy chromaticity;
    double correlated_temperature;
} cp_illuminant_info;

typedef struct cp_rgb_working_space {
    const char *name;
    cp_xyy white;
    cp_xyy red;
    cp_xyy green;
    cp_xyy blue;
    double gamma;
} cp_rgb_working_space;

typedef struct cp_named_color_value {
    const char *name;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} cp_named_color_value;

typedef struct cp_profile_difference {
    size_t matching_tags;
    size_t missing_from_left;
    size_t missing_from_right;
    size_t differing_payloads;
    int header_equal;
    double white_point_distance;
} cp_profile_difference;

typedef struct cp_diagnostic {
    int severity;
    uint32_t tag_signature;
    char message[160];
} cp_diagnostic;

typedef struct cp_gamut_summary {
    cp_lab minimum;
    cp_lab maximum;
    cp_lab average;
    double volume_estimate;
    size_t sample_count;
} cp_gamut_summary;

void cp_parse_options_init(cp_parse_options *options);
cp_status cp_profile_open_memory(const uint8_t *data, size_t size,
                                 const cp_parse_options *options,
                                 cp_profile **out_profile);
cp_status cp_profile_open_file(const char *path, const cp_parse_options *options,
                               cp_profile **out_profile);
void cp_profile_close(cp_profile *profile);
const char *cp_profile_error(const cp_profile *profile);
cp_status cp_profile_header(const cp_profile *profile, cp_header_info *out);
size_t cp_profile_tag_count(const cp_profile *profile);
cp_status cp_profile_tag_at(const cp_profile *profile, size_t index, cp_tag_info *out);
cp_status cp_profile_find_tag(const cp_profile *profile, uint32_t signature, cp_tag_info *out);
cp_status cp_profile_tag_data(const cp_profile *profile, uint32_t signature,
                              const uint8_t **data, size_t *size);
cp_status cp_profile_description(const cp_profile *profile, char *buffer, size_t capacity);
cp_status cp_profile_media_white_point(const cp_profile *profile, cp_xyz *out);
cp_status cp_profile_compute_id(const cp_profile *profile, uint8_t digest[16]);
cp_status cp_profile_compare(const cp_profile *left, const cp_profile *right,
                             cp_profile_difference *difference);
size_t cp_profile_diagnostics(const cp_profile *profile, cp_diagnostic *items,
                              size_t capacity);
cp_status cp_profile_format_report(const cp_profile *profile, char *buffer,
                                   size_t capacity, size_t *written);
cp_status cp_profile_format_json(const cp_profile *profile, char *buffer,
                                 size_t capacity, size_t *written);
cp_status cp_profile_format_csv(const cp_profile *profile, char *buffer,
                                size_t capacity, size_t *written);

cp_status cp_transform_create(const cp_profile *profile, uint32_t intent,
                              cp_transform **out_transform);
void cp_transform_destroy(cp_transform *transform);
cp_status cp_transform_apply_rgb(const cp_transform *transform,
                                 const double input[3], double output[3]);
cp_status cp_transform_apply_rgb_batch(const cp_transform *transform,
                                       const double *input, double *output,
                                       size_t pixel_count, size_t stride);

cp_pipeline *cp_pipeline_create(size_t input_channels, size_t output_channels);
void cp_pipeline_destroy(cp_pipeline *pipeline);
cp_status cp_pipeline_append_matrix(cp_pipeline *pipeline,
                                    const cp_matrix3x3 *matrix);
cp_status cp_pipeline_append_gamma(cp_pipeline *pipeline,
                                   const double gamma[3]);
cp_status cp_pipeline_apply(const cp_pipeline *pipeline, const double input[3],
                            double output[3]);
size_t cp_pipeline_stage_count(const cp_pipeline *pipeline);

cp_clut *cp_clut_create(size_t input_channels, size_t output_channels,
                        const size_t *grid_points);
void cp_clut_destroy(cp_clut *clut);
cp_status cp_clut_set(cp_clut *clut, const size_t *coordinates,
                      const double *values);
cp_status cp_clut_get(const cp_clut *clut, const size_t *coordinates,
                      double *values);
cp_status cp_clut_evaluate(const cp_clut *clut, const double *input,
                           cp_interpolation_method method, double *output);
size_t cp_clut_cell_count(const cp_clut *clut);

uint32_t cp_signature(const char text[4]);
void cp_signature_text(uint32_t signature, char text[5]);
const char *cp_signature_name(uint32_t signature);
cp_signature_group cp_signature_group_of(uint32_t signature);
size_t cp_signature_registry_count(void);
cp_status cp_signature_registry_at(size_t index, cp_signature_info *out);
const char *cp_status_string(cp_status status);

const char *cp_color_space_name(uint32_t signature);
size_t cp_color_space_channels(uint32_t signature);
int cp_color_space_is_pcs(uint32_t signature);
int cp_color_space_is_device(uint32_t signature);

int cp_datetime_valid(const cp_datetime *value);
int cp_datetime_compare(const cp_datetime *left, const cp_datetime *right);
cp_status cp_datetime_format_iso8601(const cp_datetime *value, char *buffer,
                                     size_t capacity);
cp_status cp_datetime_parse_icc(const uint8_t data[12], cp_datetime *out);

cp_matrix3x3 cp_matrix3x3_identity(void);
cp_matrix3x3 cp_matrix3x3_multiply(cp_matrix3x3 left, cp_matrix3x3 right);
cp_xyz cp_matrix3x3_apply(cp_matrix3x3 matrix, cp_xyz value);
double cp_matrix3x3_determinant(cp_matrix3x3 matrix);
cp_status cp_matrix3x3_inverse(cp_matrix3x3 matrix, cp_matrix3x3 *out);
cp_status cp_chromatic_adaptation_matrix(cp_xyz source_white,
                                         cp_xyz destination_white,
                                         cp_adaptation_method method,
                                         cp_matrix3x3 *out);
cp_xyz cp_chromatic_adapt_xyz(cp_xyz value, cp_xyz source_white,
                              cp_xyz destination_white,
                              cp_adaptation_method method);

cp_xyy cp_xyz_to_xyy(cp_xyz value);
cp_xyz cp_xyy_to_xyz(cp_xyy value);
cp_lab cp_xyz_to_lab(cp_xyz value, cp_xyz white_point);
cp_xyz cp_lab_to_xyz(cp_lab value, cp_xyz white_point);
cp_lch cp_lab_to_lch(cp_lab value);
cp_lab cp_lch_to_lab(cp_lch value);
cp_rgb cp_xyz_to_srgb(cp_xyz value);
cp_xyz cp_srgb_to_xyz(cp_rgb value);
cp_hsl cp_rgb_to_hsl(cp_rgb value);
cp_rgb cp_hsl_to_rgb(cp_hsl value);
cp_hsv cp_rgb_to_hsv(cp_rgb value);
cp_rgb cp_hsv_to_rgb(cp_hsv value);
cp_cmyk cp_rgb_to_cmyk(cp_rgb value);
cp_rgb cp_cmyk_to_rgb(cp_cmyk value);

double cp_delta_e_1976(cp_lab left, cp_lab right);
double cp_delta_e_1994_graphics(cp_lab left, cp_lab right);
double cp_delta_e_1994_textiles(cp_lab left, cp_lab right);
double cp_delta_e_2000(cp_lab left, cp_lab right);
double cp_delta_e_cmc(cp_lab left, cp_lab right, double lightness,
                      double chroma);

double cp_interpolate_linear(double left, double right, double amount);
double cp_interpolate_cubic(double p0, double p1, double p2, double p3,
                            double amount);
cp_status cp_resample_uniform(const double *source, size_t source_count,
                              double *destination, size_t destination_count);
cp_status cp_monotonic_cubic_slopes(const double *x, const double *y,
                                    size_t count, double *slopes);
double cp_monotonic_cubic_eval(const double *x, const double *y,
                               const double *slopes, size_t count, double value);

void cp_statistics_begin(cp_statistics_accumulator *accumulator);
void cp_statistics_add(cp_statistics_accumulator *accumulator, double value);
cp_status cp_statistics_finish(const cp_statistics_accumulator *accumulator,
                               cp_statistics *out);
cp_status cp_statistics_compute(const double *values, size_t count,
                                cp_statistics *out);

double cp_cie1931_x(double wavelength_nm);
double cp_cie1931_y(double wavelength_nm);
double cp_cie1931_z(double wavelength_nm);
double cp_planck_spd(double wavelength_nm, double temperature_kelvin);
double cp_daylight_spd(double wavelength_nm, double correlated_temperature);
cp_status cp_spectral_interpolate(const cp_spectral_curve *curve,
                                  double wavelength_nm, double *out);
cp_status cp_spectral_to_xyz(const cp_spectral_curve *reflectance,
                             const cp_spectral_curve *illuminant, cp_xyz *out);

size_t cp_illuminant_count(void);
cp_status cp_illuminant_at(size_t index, cp_illuminant_info *out);
cp_status cp_illuminant_find(const char *name, cp_illuminant_info *out);
cp_xyz cp_illuminant_xyz(const cp_illuminant_info *illuminant);
double cp_xyy_correlated_temperature(cp_xyy chromaticity);
cp_xyy cp_daylight_chromaticity(double correlated_temperature);
cp_xyy cp_blackbody_chromaticity(double temperature_kelvin);

size_t cp_rgb_working_space_count(void);
cp_status cp_rgb_working_space_at(size_t index, cp_rgb_working_space *out);
cp_status cp_rgb_working_space_find(const char *name, cp_rgb_working_space *out);
cp_status cp_rgb_working_space_matrix(const cp_rgb_working_space *space,
                                      cp_matrix3x3 *rgb_to_xyz,
                                      cp_matrix3x3 *xyz_to_rgb);

double cp_tone_map_reinhard(double value, double white);
double cp_tone_map_hable(double value);
double cp_tone_map_aces(double value);
cp_rgb cp_tone_map_rgb(cp_rgb value, double exposure, int operator_id);

size_t cp_named_color_count(void);
cp_status cp_named_color_at(size_t index, cp_named_color_value *out);
cp_status cp_named_color_find(const char *name, cp_named_color_value *out);
cp_status cp_named_color_nearest(cp_rgb value, cp_named_color_value *out,
                                 double *distance);

size_t cp_reference_grid_count(void);
cp_status cp_reference_grid_at(size_t index, cp_reference_color *out);
cp_status cp_reference_grid_nearest(cp_lab value, cp_reference_color *out,
                                    double *distance);
cp_status cp_reference_grid_quantize(cp_rgb value, cp_rgb *out);

cp_status cp_gamut_analyze_rgb_transform(const cp_transform *transform,
                                         size_t steps, cp_gamut_summary *out);
cp_status cp_gamut_contains_lab(const cp_gamut_summary *summary, cp_lab value,
                                int *inside);

cp_tone_curve *cp_tone_curve_create_gamma(double gamma);
cp_tone_curve *cp_tone_curve_create_table(const double *values, size_t count);
void cp_tone_curve_destroy(cp_tone_curve *curve);
double cp_tone_curve_evaluate(const cp_tone_curve *curve, double input);
cp_status cp_tone_curve_invert(const cp_tone_curve *curve, double output,
                               double *input);
cp_status cp_tone_curve_sample(const cp_tone_curve *curve, double *values,
                               size_t count);
size_t cp_tone_curve_sample_count(const cp_tone_curve *curve);

cp_status cp_utf8_validate(const char *text, size_t length, size_t *codepoints);
cp_status cp_utf16be_to_utf8(const uint8_t *source, size_t source_size,
                             char *destination, size_t capacity, size_t *written);
cp_status cp_utf8_normalize_line_endings(const char *source, size_t source_size,
                                         char *destination, size_t capacity,
                                         size_t *written);

uint8_t cp_quantize_u8(double value);
uint16_t cp_quantize_u16(double value);
double cp_dequantize_u8(uint8_t value);
double cp_dequantize_u16(uint16_t value);
cp_status cp_encode_lab8(cp_lab value, uint8_t encoded[3]);
cp_status cp_decode_lab8(const uint8_t encoded[3], cp_lab *value);
cp_status cp_encode_lab16(cp_lab value, uint16_t encoded[3]);
cp_status cp_decode_lab16(const uint16_t encoded[3], cp_lab *value);
cp_status cp_encode_xyz16(cp_xyz value, uint16_t encoded[3]);
cp_status cp_decode_xyz16(const uint16_t encoded[3], cp_xyz *value);

void cp_md5(const uint8_t *data, size_t size, uint8_t digest[16]);
uint32_t cp_crc32(const uint8_t *data, size_t size);
uint64_t cp_fnv1a64(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
