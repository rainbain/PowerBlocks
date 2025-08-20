/**
 * @file matrix3.h
 * @brief 3x3 Matrices
 *
 * Your standard column major 3x3 matrix you all know and love.
 * Useful for normals!
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include "powerblocks/core/utils/math/matrix34.h"

typedef float matrix3[3][3];

#define MATRIX3_COPY(dst, src) memcpy(dst, src, sizeof(matrix3));

extern void matrix3_identity(matrix3 mtx);
extern void matrix3_from34(matrix3 dst, const matrix34 src);

extern void matrix3_transpose(matrix3 dst, const matrix3 src);
extern void matrix3_inverse(matrix3 dst, const matrix3 src);