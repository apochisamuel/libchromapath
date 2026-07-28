#include "chromapath.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef enum cp_pipeline_stage_kind {
    CP_PIPELINE_MATRIX = 1,
    CP_PIPELINE_GAMMA = 2
} cp_pipeline_stage_kind;

typedef struct cp_pipeline_stage {
    cp_pipeline_stage_kind kind;
    union {
        cp_matrix3x3 matrix;
        double gamma[3];
    } value;
} cp_pipeline_stage;

struct cp_pipeline {
    size_t input_channels;
    size_t output_channels;
    cp_pipeline_stage *stages;
    size_t stage_count;
    size_t stage_capacity;
};

static cp_status cp_pipeline_reserve(cp_pipeline *pipeline, size_t count) {
    if (!pipeline) return CP_ERR_ARGUMENT;
    if (count <= pipeline->stage_capacity) return CP_OK;
    size_t capacity = pipeline->stage_capacity ? pipeline->stage_capacity : 4;
    while (capacity < count) {
        if (capacity > SIZE_MAX / 2) return CP_ERR_OVERFLOW;
        capacity *= 2;
    }
    cp_pipeline_stage *stages = (cp_pipeline_stage *)realloc(
        pipeline->stages, capacity * sizeof(*stages));
    if (!stages) return CP_ERR_NOMEM;
    pipeline->stages = stages;
    pipeline->stage_capacity = capacity;
    return CP_OK;
}

cp_pipeline *cp_pipeline_create(size_t input_channels, size_t output_channels) {
    if (input_channels != 3 || output_channels != 3) return NULL;
    cp_pipeline *pipeline = (cp_pipeline *)calloc(1, sizeof(*pipeline));
    if (!pipeline) return NULL;
    pipeline->input_channels = input_channels;
    pipeline->output_channels = output_channels;
    return pipeline;
}

void cp_pipeline_destroy(cp_pipeline *pipeline) {
    if (!pipeline) return;
    free(pipeline->stages);
    free(pipeline);
}

cp_status cp_pipeline_append_matrix(cp_pipeline *pipeline,
                                    const cp_matrix3x3 *matrix) {
    if (!pipeline || !matrix) return CP_ERR_ARGUMENT;
    cp_status status = cp_pipeline_reserve(pipeline, pipeline->stage_count + 1);
    if (status != CP_OK) return status;
    cp_pipeline_stage *stage = &pipeline->stages[pipeline->stage_count++];
    memset(stage, 0, sizeof(*stage));
    stage->kind = CP_PIPELINE_MATRIX;
    stage->value.matrix = *matrix;
    return CP_OK;
}

cp_status cp_pipeline_append_gamma(cp_pipeline *pipeline,
                                   const double gamma[3]) {
    if (!pipeline || !gamma) return CP_ERR_ARGUMENT;
    for (size_t i = 0; i < 3; ++i) {
        if (!isfinite(gamma[i]) || gamma[i] <= 0.0) return CP_ERR_FORMAT;
    }
    cp_status status = cp_pipeline_reserve(pipeline, pipeline->stage_count + 1);
    if (status != CP_OK) return status;
    cp_pipeline_stage *stage = &pipeline->stages[pipeline->stage_count++];
    memset(stage, 0, sizeof(*stage));
    stage->kind = CP_PIPELINE_GAMMA;
    memcpy(stage->value.gamma, gamma, sizeof(stage->value.gamma));
    return CP_OK;
}

static void cp_pipeline_apply_matrix(const cp_matrix3x3 *matrix,
                                     const double input[3], double output[3]) {
    output[0] = matrix->v[0][0] * input[0] + matrix->v[0][1] * input[1] +
                matrix->v[0][2] * input[2];
    output[1] = matrix->v[1][0] * input[0] + matrix->v[1][1] * input[1] +
                matrix->v[1][2] * input[2];
    output[2] = matrix->v[2][0] * input[0] + matrix->v[2][1] * input[1] +
                matrix->v[2][2] * input[2];
}

static void cp_pipeline_apply_gamma(const double gamma[3],
                                    const double input[3], double output[3]) {
    for (size_t i = 0; i < 3; ++i) {
        double value = input[i];
        if (value < 0.0) value = 0.0;
        output[i] = pow(value, gamma[i]);
    }
}

cp_status cp_pipeline_apply(const cp_pipeline *pipeline, const double input[3],
                            double output[3]) {
    if (!pipeline || !input || !output) return CP_ERR_ARGUMENT;
    double current[3] = {input[0], input[1], input[2]};
    double next[3] = {0.0, 0.0, 0.0};
    for (size_t i = 0; i < pipeline->stage_count; ++i) {
        const cp_pipeline_stage *stage = &pipeline->stages[i];
        if (stage->kind == CP_PIPELINE_MATRIX) {
            cp_pipeline_apply_matrix(&stage->value.matrix, current, next);
        } else if (stage->kind == CP_PIPELINE_GAMMA) {
            cp_pipeline_apply_gamma(stage->value.gamma, current, next);
        } else {
            return CP_ERR_STATE;
        }
        memcpy(current, next, sizeof(current));
    }
    memcpy(output, current, sizeof(current));
    return CP_OK;
}

size_t cp_pipeline_stage_count(const cp_pipeline *pipeline) {
    return pipeline ? pipeline->stage_count : 0;
}
