/**
 * @file vec3.h
 * @brief Simple 3D vectors.
 *
 * Provides helper functions and data structures for 3D vectors.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

typedef struct {
    float x;
    float y;
    float z;
} vec3;

#define vec3_new(x, y, z) ((vec3){x, y, z})