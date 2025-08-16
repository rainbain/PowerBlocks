/**
 * @file matrix4.c
 * @brief 4x4 Matrices
 *
 * Your standard column major 4x4 matrix you all know and love.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "matrix4.h"

#include <math.h>

void matrix4_perspective(matrix4 mtx, float fov, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov * 0.5f);

    mtx[0][0] = f / aspect;
    mtx[0][1] = 0.0f;
    mtx[0][2] = 0.0f;
    mtx[0][3] = 0.0f;

    mtx[1][0] = 0.0f;
    mtx[1][1] = f;
    mtx[1][2] = 0.0f;
    mtx[1][3] = 0.0f;

    mtx[2][0] = 0.0f;
    mtx[2][1] = 0.0f;
    mtx[2][2] = near / (near - far);
    mtx[2][3] = (far * near) / (near - far);

    mtx[3][0] = 0.0f;
    mtx[3][1] = 0.0f;
    mtx[3][2] = -1.0f;
    mtx[3][3] = 0.0f;
}

void matrix4_orthographic(matrix4 mtx, float left, float right, float bottom, float top, float near, float far) {
    mtx[0][0] = 2.0f / (right - left);
    mtx[0][1] = 0.0f;
    mtx[0][2] = 0.0f;
    mtx[0][3] = -(right + left) / (right - left);

    mtx[1][0] = 0.0f;
    mtx[1][1] = 2.0f / (top - bottom);
    mtx[1][2] = 0.0f;
    mtx[1][3] = -(top + bottom) / (top - bottom);

    mtx[2][0] = 0.0f;
    mtx[2][1] = 0.0f;
    mtx[2][2] = -2.0f / (far - near);
    mtx[2][3] = -(far + near) / (far - near);

    mtx[3][0] = 0.0f;
    mtx[3][1] = 0.0f;
    mtx[3][2] = 0.0f;
    mtx[3][3] = 1.0f;
}