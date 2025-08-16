/**
 * @file matrix4.h
 * @brief 3x4 Matrices
 *
 * Your standard column major 3x4 matrix.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "matrix34.h"

#include <math.h>

void matrix34_identity(matrix34 mtx) {
    mtx[0][0] = 1.0f;
    mtx[0][1] = 0.0f;
    mtx[0][2] = 0.0f;
    mtx[0][3] = 0.0f;

    mtx[1][0] = 0.0f;
    mtx[1][1] = 1.0f;
    mtx[1][2] = 0.0f;
    mtx[1][3] = 0.0f;

    mtx[2][0] = 0.0f;
    mtx[2][1] = 0.0f;
    mtx[2][2] = 1.0f;
    mtx[2][3] = 0.0f;
}

void matrix34_rotate_x(matrix34 mtx, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    mtx[0][0] = 1.0f; mtx[0][1] = 0.0f; mtx[0][2] = 0.0f; mtx[0][3] = 0.0f;
    mtx[1][0] = 0.0f; mtx[1][1] = c;    mtx[1][2] = -s;   mtx[1][3] = 0.0f;
    mtx[2][0] = 0.0f; mtx[2][1] = s;    mtx[2][2] = c;    mtx[2][3] = 0.0f;
}

void matrix34_rotate_y(matrix34 mtx, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    mtx[0][0] = c;    mtx[0][1] = 0.0f; mtx[0][2] = s;    mtx[0][3] = 0.0f;
    mtx[1][0] = 0.0f; mtx[1][1] = 1.0f; mtx[1][2] = 0.0f; mtx[1][3] = 0.0f;
    mtx[2][0] = -s;   mtx[2][1] = 0.0f; mtx[2][2] = c;    mtx[2][3] = 0.0f;
}

void matrix34_rotate_z(matrix34 mtx, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    mtx[0][0] = c;    mtx[0][1] = -s;   mtx[0][2] = 0.0f; mtx[0][3] = 0.0f;
    mtx[1][0] = s;    mtx[1][1] = c;    mtx[1][2] = 0.0f; mtx[1][3] = 0.0f;
    mtx[2][0] = 0.0f; mtx[2][1] = 0.0f; mtx[2][2] = 1.0f; mtx[2][3] = 0.0f;
}

void matrix34_translation(matrix34 mtx, const vec3* pos) {
    mtx[0][0] = 1.0f; mtx[0][1] = 0.0f; mtx[0][2] = 0.0f; mtx[0][3] = pos->x;
    mtx[1][0] = 0.0f; mtx[1][1] = 1.0f; mtx[1][2] = 0.0f; mtx[1][3] = pos->y;
    mtx[2][0] = 0.0f; mtx[2][1] = 0.0f; mtx[2][2] = 1.0f; mtx[2][3] = pos->z;
}

void matrix34_scale(matrix34 mtx, const vec3* scale) {
    mtx[0][0] = scale->x;    mtx[0][1] = 0.0f;        mtx[0][2] = 0.0f;        mtx[0][3] = 0.0f;
    mtx[1][0] = 0.0f;        mtx[1][1] = scale->y;    mtx[1][2] = 0.0f;        mtx[1][3] = 0.0f;
    mtx[2][0] = 0.0f;        mtx[2][1] = 0.0f;        mtx[2][2] = scale->z;    mtx[2][3] = 0.0f;
}

void matrix34_multiply(matrix34 dst, const matrix34 a, const matrix34 b) {
    matrix34 tmp;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            tmp[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
        }
    }

    for (int i = 0; i < 3; ++i) {
        tmp[i][3] = a[i][0] * b[0][3] + a[i][1] * b[1][3] + a[i][2] * b[2][3] + a[i][3];
    }

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            dst[i][j] = tmp[i][j];
}

void matrix34_inverse(matrix34 dst, const matrix34 src) {
    // Transpose rotation part
    dst[0][0] = src[0][0];
    dst[0][1] = src[1][0];
    dst[0][2] = src[2][0];

    dst[1][0] = src[0][1];
    dst[1][1] = src[1][1];
    dst[1][2] = src[2][1];

    dst[2][0] = src[0][2];
    dst[2][1] = src[1][2];
    dst[2][2] = src[2][2];

    // Translation = -(R^T * t)
    dst[0][3] = -(dst[0][0] * src[0][3] + dst[0][1] * src[1][3] + dst[0][2] * src[2][3]);
    dst[1][3] = -(dst[1][0] * src[0][3] + dst[1][1] * src[1][3] + dst[1][2] * src[2][3]);
    dst[2][3] = -(dst[2][0] * src[0][3] + dst[2][1] * src[1][3] + dst[2][2] * src[2][3]);
}
