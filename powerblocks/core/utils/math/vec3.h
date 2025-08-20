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

#pragma once

#include <math.h>

typedef struct {
    float x;
    float y;
    float z;
} vec3;

#define vec3_new(x, y, z) ((vec3){x, y, z})
#define vec3_add(a, b) ((vec3){(a).x+(b).x, (a).y+(b).y, (a).z+(b).z})
#define vec3_sub(a, b) ((vec3){(a).x-(b).x, (a).y-(b).y, (a).z-(b).z})
#define vec3_dot(a, b) ((a).x * (b).x + (a).y * (b).y + (a.z) * (b).z)
#define vec3_magnitude_sq(a) vec3_dot(a, a)
#define vec3_magnitude(a) (sqrtf(vec3_magnitude_sq(a)))

#define vec3_muls(a, s) ((vec3){(a).x*(s), (a).y*(s), (a).z*(s)})
#define vec3_divs(a, s) ((vec3){(a).x/(s), (a).y/(s), (a).z/(s)})

extern void vec3_normalize(vec3* v);

// Generates a unit vector pointing from A -> B
extern vec3 vec3_to(vec3 a, vec3 b);