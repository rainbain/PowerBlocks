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

#include "vec3.h"

void vec3_normalize(vec3* v) {
    float l = vec3_magnitude(v[0]);
    v[0] = vec3_divs(v[0], l);
}

vec3 vec3_to(vec3 a, vec3 b) {
    vec3 v = vec3_sub(b, a);
    vec3_normalize(&v);
    return v;
}