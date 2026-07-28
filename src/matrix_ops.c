#include "chromapath.h"

#include <math.h>

cp_matrix3x3 cp_matrix3x3_identity(void) {
    cp_matrix3x3 result = {{{1.0, 0.0, 0.0},
                            {0.0, 1.0, 0.0},
                            {0.0, 0.0, 1.0}}};
    return result;
}

cp_matrix3x3 cp_matrix3x3_multiply(cp_matrix3x3 left, cp_matrix3x3 right) {
    cp_matrix3x3 result = {{{0.0, 0.0, 0.0},
                            {0.0, 0.0, 0.0},
                            {0.0, 0.0, 0.0}}};
    for (size_t row = 0; row < 3; ++row) {
        for (size_t column = 0; column < 3; ++column) {
            for (size_t term = 0; term < 3; ++term) {
                result.v[row][column] += left.v[row][term] * right.v[term][column];
            }
        }
    }
    return result;
}

cp_xyz cp_matrix3x3_apply(cp_matrix3x3 matrix, cp_xyz value) {
    cp_xyz result;
    result.x = matrix.v[0][0] * value.x + matrix.v[0][1] * value.y +
               matrix.v[0][2] * value.z;
    result.y = matrix.v[1][0] * value.x + matrix.v[1][1] * value.y +
               matrix.v[1][2] * value.z;
    result.z = matrix.v[2][0] * value.x + matrix.v[2][1] * value.y +
               matrix.v[2][2] * value.z;
    return result;
}

double cp_matrix3x3_determinant(cp_matrix3x3 matrix) {
    double a = matrix.v[0][0];
    double b = matrix.v[0][1];
    double c = matrix.v[0][2];
    double d = matrix.v[1][0];
    double e = matrix.v[1][1];
    double f = matrix.v[1][2];
    double g = matrix.v[2][0];
    double h = matrix.v[2][1];
    double i = matrix.v[2][2];
    return a * (e * i - f * h) - b * (d * i - f * g) +
           c * (d * h - e * g);
}

cp_status cp_matrix3x3_inverse(cp_matrix3x3 matrix, cp_matrix3x3 *out) {
    if (!out) return CP_ERR_ARGUMENT;
    double determinant = cp_matrix3x3_determinant(matrix);
    if (fabs(determinant) < 1e-15) return CP_ERR_FORMAT;
    double reciprocal = 1.0 / determinant;
    cp_matrix3x3 result;
    result.v[0][0] = (matrix.v[1][1] * matrix.v[2][2] - matrix.v[1][2] * matrix.v[2][1]) * reciprocal;
    result.v[0][1] = (matrix.v[0][2] * matrix.v[2][1] - matrix.v[0][1] * matrix.v[2][2]) * reciprocal;
    result.v[0][2] = (matrix.v[0][1] * matrix.v[1][2] - matrix.v[0][2] * matrix.v[1][1]) * reciprocal;
    result.v[1][0] = (matrix.v[1][2] * matrix.v[2][0] - matrix.v[1][0] * matrix.v[2][2]) * reciprocal;
    result.v[1][1] = (matrix.v[0][0] * matrix.v[2][2] - matrix.v[0][2] * matrix.v[2][0]) * reciprocal;
    result.v[1][2] = (matrix.v[0][2] * matrix.v[1][0] - matrix.v[0][0] * matrix.v[1][2]) * reciprocal;
    result.v[2][0] = (matrix.v[1][0] * matrix.v[2][1] - matrix.v[1][1] * matrix.v[2][0]) * reciprocal;
    result.v[2][1] = (matrix.v[0][1] * matrix.v[2][0] - matrix.v[0][0] * matrix.v[2][1]) * reciprocal;
    result.v[2][2] = (matrix.v[0][0] * matrix.v[1][1] - matrix.v[0][1] * matrix.v[1][0]) * reciprocal;
    *out = result;
    return CP_OK;
}

static cp_matrix3x3 cp_adaptation_basis(cp_adaptation_method method) {
    switch (method) {
        case CP_ADAPT_BRADFORD:
            return (cp_matrix3x3){{{0.8951000, 0.2664000, -0.1614000},
                                   {-0.7502000, 1.7135000, 0.0367000},
                                   {0.0389000, -0.0685000, 1.0296000}}};
        case CP_ADAPT_CAT02:
            return (cp_matrix3x3){{{0.7328000, 0.4296000, -0.1624000},
                                   {-0.7036000, 1.6975000, 0.0061000},
                                   {0.0030000, 0.0136000, 0.9834000}}};
        case CP_ADAPT_VON_KRIES:
            return (cp_matrix3x3){{{0.4002400, 0.7076000, -0.0808100},
                                   {-0.2263000, 1.1653200, 0.0457000},
                                   {0.0000000, 0.0000000, 0.9182200}}};
        case CP_ADAPT_XYZ_SCALING:
        default:
            return cp_matrix3x3_identity();
    }
}

cp_status cp_chromatic_adaptation_matrix(cp_xyz source_white,
                                         cp_xyz destination_white,
                                         cp_adaptation_method method,
                                         cp_matrix3x3 *out) {
    if (!out) return CP_ERR_ARGUMENT;
    cp_matrix3x3 basis = cp_adaptation_basis(method);
    cp_matrix3x3 inverse;
    cp_status status = cp_matrix3x3_inverse(basis, &inverse);
    if (status != CP_OK) return status;
    cp_xyz source_response = cp_matrix3x3_apply(basis, source_white);
    cp_xyz destination_response = cp_matrix3x3_apply(basis, destination_white);
    if (fabs(source_response.x) < 1e-15 || fabs(source_response.y) < 1e-15 ||
        fabs(source_response.z) < 1e-15) return CP_ERR_FORMAT;
    cp_matrix3x3 diagonal = {{{destination_response.x / source_response.x, 0.0, 0.0},
                              {0.0, destination_response.y / source_response.y, 0.0},
                              {0.0, 0.0, destination_response.z / source_response.z}}};
    *out = cp_matrix3x3_multiply(inverse, cp_matrix3x3_multiply(diagonal, basis));
    return CP_OK;
}

cp_xyz cp_chromatic_adapt_xyz(cp_xyz value, cp_xyz source_white,
                              cp_xyz destination_white,
                              cp_adaptation_method method) {
    cp_matrix3x3 matrix;
    if (cp_chromatic_adaptation_matrix(source_white, destination_white,
                                       method, &matrix) != CP_OK) return value;
    return cp_matrix3x3_apply(matrix, value);
}
