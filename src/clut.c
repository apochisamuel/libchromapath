#include "chromapath.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct cp_clut {
    size_t input_channels;
    size_t output_channels;
    size_t *grid_points;
    size_t *strides;
    size_t cell_count;
    double *values;
};

static cp_status cp_clut_compute_layout(size_t input_channels,
                                        size_t output_channels,
                                        const size_t *grid_points,
                                        size_t *strides,
                                        size_t *cell_count) {
    if (!grid_points || !strides || !cell_count) return CP_ERR_ARGUMENT;
    if (input_channels == 0 || input_channels > 16 || output_channels == 0 ||
        output_channels > 16) return CP_ERR_ARGUMENT;
    size_t cells = 1;
    for (size_t index = input_channels; index > 0; --index) {
        size_t channel = index - 1;
        size_t points = grid_points[channel];
        if (points < 2 || points > 256) return CP_ERR_FORMAT;
        strides[channel] = cells;
        if (cells > SIZE_MAX / points) return CP_ERR_OVERFLOW;
        cells *= points;
    }
    if (cells > SIZE_MAX / output_channels) return CP_ERR_OVERFLOW;
    *cell_count = cells;
    return CP_OK;
}

cp_clut *cp_clut_create(size_t input_channels, size_t output_channels,
                        const size_t *grid_points) {
    if (!grid_points) return NULL;
    cp_clut *clut = (cp_clut *)calloc(1, sizeof(*clut));
    if (!clut) return NULL;
    clut->input_channels = input_channels;
    clut->output_channels = output_channels;
    clut->grid_points = (size_t *)calloc(input_channels, sizeof(size_t));
    clut->strides = (size_t *)calloc(input_channels, sizeof(size_t));
    if (!clut->grid_points || !clut->strides) {
        cp_clut_destroy(clut);
        return NULL;
    }
    memcpy(clut->grid_points, grid_points, input_channels * sizeof(size_t));
    cp_status status = cp_clut_compute_layout(input_channels, output_channels,
                                              clut->grid_points, clut->strides,
                                              &clut->cell_count);
    if (status != CP_OK) {
        cp_clut_destroy(clut);
        return NULL;
    }
    clut->values = (double *)calloc(clut->cell_count * output_channels,
                                    sizeof(double));
    if (!clut->values) {
        cp_clut_destroy(clut);
        return NULL;
    }
    return clut;
}

void cp_clut_destroy(cp_clut *clut) {
    if (!clut) return;
    free(clut->grid_points);
    free(clut->strides);
    free(clut->values);
    free(clut);
}

static cp_status cp_clut_offset(const cp_clut *clut, const size_t *coordinates,
                                size_t *offset) {
    if (!clut || !coordinates || !offset) return CP_ERR_ARGUMENT;
    size_t result = 0;
    for (size_t channel = 0; channel < clut->input_channels; ++channel) {
        if (coordinates[channel] >= clut->grid_points[channel]) {
            return CP_ERR_NOT_FOUND;
        }
        result += coordinates[channel] * clut->strides[channel];
    }
    *offset = result;
    return CP_OK;
}

cp_status cp_clut_set(cp_clut *clut, const size_t *coordinates,
                      const double *values) {
    if (!clut || !values) return CP_ERR_ARGUMENT;
    size_t offset;
    cp_status status = cp_clut_offset(clut, coordinates, &offset);
    if (status != CP_OK) return status;
    memcpy(clut->values + offset * clut->output_channels, values,
           clut->output_channels * sizeof(double));
    return CP_OK;
}

cp_status cp_clut_get(const cp_clut *clut, const size_t *coordinates,
                      double *values) {
    if (!clut || !values) return CP_ERR_ARGUMENT;
    size_t offset;
    cp_status status = cp_clut_offset(clut, coordinates, &offset);
    if (status != CP_OK) return status;
    memcpy(values, clut->values + offset * clut->output_channels,
           clut->output_channels * sizeof(double));
    return CP_OK;
}

static cp_status cp_clut_evaluate_nearest(const cp_clut *clut,
                                          const double *input,
                                          double *output) {
    size_t coordinates[16];
    for (size_t channel = 0; channel < clut->input_channels; ++channel) {
        double value = input[channel];
        if (value < 0.0) value = 0.0;
        if (value > 1.0) value = 1.0;
        double position = value * (double)(clut->grid_points[channel] - 1);
        coordinates[channel] = (size_t)floor(position + 0.5);
    }
    return cp_clut_get(clut, coordinates, output);
}

static cp_status cp_clut_evaluate_linear(const cp_clut *clut,
                                         const double *input,
                                         double *output) {
    if (clut->input_channels > 12) return CP_ERR_UNSUPPORTED;
    size_t lower[16];
    size_t upper[16];
    double amount[16];
    for (size_t channel = 0; channel < clut->input_channels; ++channel) {
        double value = input[channel];
        if (value < 0.0) value = 0.0;
        if (value > 1.0) value = 1.0;
        double position = value * (double)(clut->grid_points[channel] - 1);
        lower[channel] = (size_t)floor(position);
        upper[channel] = lower[channel] + 1;
        if (upper[channel] >= clut->grid_points[channel]) upper[channel] = lower[channel];
        amount[channel] = position - (double)lower[channel];
    }
    for (size_t output_channel = 0; output_channel < clut->output_channels;
         ++output_channel) output[output_channel] = 0.0;
    size_t corners = (size_t)1 << clut->input_channels;
    size_t coordinates[16];
    for (size_t corner = 0; corner < corners; ++corner) {
        double weight = 1.0;
        for (size_t channel = 0; channel < clut->input_channels; ++channel) {
            if (corner & ((size_t)1 << channel)) {
                coordinates[channel] = upper[channel];
                weight *= amount[channel];
            } else {
                coordinates[channel] = lower[channel];
                weight *= 1.0 - amount[channel];
            }
        }
        if (weight == 0.0) continue;
        size_t offset;
        cp_status status = cp_clut_offset(clut, coordinates, &offset);
        if (status != CP_OK) return status;
        const double *sample = clut->values + offset * clut->output_channels;
        for (size_t output_channel = 0; output_channel < clut->output_channels;
             ++output_channel) {
            output[output_channel] += sample[output_channel] * weight;
        }
    }
    return CP_OK;
}

static void cp_sort_fraction_order(const double *fraction, size_t count,
                                   size_t *order) {
    for (size_t i = 0; i < count; ++i) order[i] = i;
    for (size_t i = 1; i < count; ++i) {
        size_t value = order[i];
        size_t j = i;
        while (j > 0 && fraction[order[j - 1]] < fraction[value]) {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = value;
    }
}

static cp_status cp_clut_evaluate_tetrahedral(const cp_clut *clut,
                                              const double *input,
                                              double *output) {
    if (clut->input_channels != 3) return CP_ERR_UNSUPPORTED;
    size_t lower[3];
    size_t upper[3];
    double fraction[3];
    for (size_t channel = 0; channel < 3; ++channel) {
        double value = input[channel];
        if (value < 0.0) value = 0.0;
        if (value > 1.0) value = 1.0;
        double position = value * (double)(clut->grid_points[channel] - 1);
        lower[channel] = (size_t)floor(position);
        upper[channel] = lower[channel] + 1;
        if (upper[channel] >= clut->grid_points[channel]) upper[channel] = lower[channel];
        fraction[channel] = position - (double)lower[channel];
    }
    size_t order[3];
    cp_sort_fraction_order(fraction, 3, order);
    size_t vertices[4][3];
    for (size_t channel = 0; channel < 3; ++channel) vertices[0][channel] = lower[channel];
    for (size_t vertex = 1; vertex < 4; ++vertex) {
        memcpy(vertices[vertex], vertices[vertex - 1], sizeof(vertices[vertex]));
        vertices[vertex][order[vertex - 1]] = upper[order[vertex - 1]];
    }
    double weights[4];
    weights[0] = 1.0 - fraction[order[0]];
    weights[1] = fraction[order[0]] - fraction[order[1]];
    weights[2] = fraction[order[1]] - fraction[order[2]];
    weights[3] = fraction[order[2]];
    for (size_t output_channel = 0; output_channel < clut->output_channels;
         ++output_channel) output[output_channel] = 0.0;
    for (size_t vertex = 0; vertex < 4; ++vertex) {
        size_t offset;
        cp_status status = cp_clut_offset(clut, vertices[vertex], &offset);
        if (status != CP_OK) return status;
        const double *sample = clut->values + offset * clut->output_channels;
        for (size_t output_channel = 0; output_channel < clut->output_channels;
             ++output_channel) {
            output[output_channel] += weights[vertex] * sample[output_channel];
        }
    }
    return CP_OK;
}

cp_status cp_clut_evaluate(const cp_clut *clut, const double *input,
                           cp_interpolation_method method, double *output) {
    if (!clut || !input || !output) return CP_ERR_ARGUMENT;
    switch (method) {
        case CP_INTERPOLATE_NEAREST:
            return cp_clut_evaluate_nearest(clut, input, output);
        case CP_INTERPOLATE_LINEAR:
            return cp_clut_evaluate_linear(clut, input, output);
        case CP_INTERPOLATE_TETRAHEDRAL:
            return cp_clut_evaluate_tetrahedral(clut, input, output);
        default:
            return CP_ERR_UNSUPPORTED;
    }
}

size_t cp_clut_cell_count(const cp_clut *clut) {
    return clut ? clut->cell_count : 0;
}
