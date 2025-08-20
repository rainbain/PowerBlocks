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

#include "vec3.h"

#include <string.h>

typedef float matrix34[3][4];

#define MATRIX34_COPY(dst, src) memcpy(dst, src, sizeof(matrix34));

extern void matrix34_identity(matrix34 mtx);

extern void matrix34_rotate_x(matrix34 mtx, float angle);
extern void matrix34_rotate_y(matrix34 mtx, float angle);
extern void matrix34_rotate_z(matrix34 mtx, float angle);

extern void matrix34_translation(matrix34 mtx, const vec3* pos);

extern void matrix34_scale(matrix34 mtx, const vec3* pos);

extern void matrix34_multiply(matrix34 dst, const matrix34 a, const matrix34 b);
extern void matrix34_inverse(matrix34 dst, const matrix34 src);