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

#include "matrix3.h"

void matrix3_identity(matrix3 mtx) {
    mtx[0][0] = 1.0f;
    mtx[0][1] = 0.0f;
    mtx[0][2] = 0.0f;

    mtx[1][0] = 0.0f;
    mtx[1][1] = 1.0f;
    mtx[1][2] = 0.0f;

    mtx[2][0] = 0.0f;
    mtx[2][1] = 0.0f;
    mtx[2][2] = 1.0f;
}

void matrix3_from34(matrix3 dst, const matrix34 src) {
    dst[0][0] = src[0][0];
    dst[0][1] = src[0][1];
    dst[0][2] = src[0][2];

    dst[1][0] = src[1][0];
    dst[1][1] = src[1][1];
    dst[1][2] = src[1][2];

    dst[2][0] = src[2][0];
    dst[2][1] = src[2][1];
    dst[2][2] = src[2][2];
}


void matrix3_transpose(matrix3 dst, const matrix3 src) {
    matrix3 tmp;

    tmp[0][0] = src[0][0];
    tmp[0][1] = src[1][0];
    tmp[0][2] = src[2][0];

    tmp[1][0] = src[0][1];
    tmp[1][1] = src[1][1];
    tmp[1][2] = src[2][1];

    tmp[2][0] = src[0][2];
    tmp[2][1] = src[1][2];
    tmp[2][2] = src[2][2];

    MATRIX3_COPY(dst, tmp);
}

void matrix3_inverse(matrix3 dst, const matrix3 src) {
    float a00 = src[0][0], a01 = src[0][1], a02 = src[0][2];
    float a10 = src[1][0], a11 = src[1][1], a12 = src[1][2];
    float a20 = src[2][0], a21 = src[2][1], a22 = src[2][2];

    float det = a00*(a11*a22 - a12*a21)
              - a01*(a10*a22 - a12*a20)
              + a02*(a10*a21 - a11*a20);

    float invDet = 1.0f / det;

    dst[0][0] =  (a11*a22 - a12*a21) * invDet;
    dst[0][1] = -(a01*a22 - a02*a21) * invDet;
    dst[0][2] =  (a01*a12 - a02*a11) * invDet;

    dst[1][0] = -(a10*a22 - a12*a20) * invDet;
    dst[1][1] =  (a00*a22 - a02*a20) * invDet;
    dst[1][2] = -(a00*a12 - a02*a10) * invDet;

    dst[2][0] =  (a10*a21 - a11*a20) * invDet;
    dst[2][1] = -(a00*a21 - a01*a20) * invDet;
    dst[2][2] =  (a00*a11 - a01*a10) * invDet;
}