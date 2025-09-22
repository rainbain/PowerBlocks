/**
 * @file vec2.h
 * @brief Simple 2D vectors.
 *
 * Provides helper functions and data structures for 2D vectors.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <math.h>
#include <stdint.h>

typedef struct {
    int x;
    int y;
} vec2i;

typedef struct {
    int16_t x;
    int16_t y;
} vec2s16;

typedef struct {
    float x;
    float y;
} vec2;

#define vec2i_new(x, y) ((vec2i){x, y})
#define vec2s16_new(x, y) ((vec2s16){x, y})
#define vec2_new(x, y) ((vec2){x, y})

#define vec2_add(a, b) ((vec2){(a).x+(b).x, (a).y+(b).y})
#define vec2_sub(a, b) ((vec2){(a).x-(b).x, (a).y-(b).y})
#define vec2_dot(a, b) ((a).x * (b).x + (a).y * (b).y)
#define vec2_magnitude_sq(a) vec2_dot(a, a)
#define vec2_magnitude(a) (sqrtf(vec2_magnitude_sq(a)))

#define vec2i_add(a, b) ((vec2i){(a).x+(b).x, (a).y+(b).y})
#define vec2i_sub(a, b) ((vec2i){(a).x-(b).x, (a).y-(b).y})
#define vec2i_dot(a, b) ((a).x * (b).x + (a).y * (b).y)
#define vec2i_magnitude_sq(a) vec2_dot(a, a)
#define vec2i_magnitude(a) (sqrtf(vec2_magnitude_sq(a)))

#define vec2s16_add(a, b) ((vec2s16){(a).x+(b).x, (a).y+(b).y})
#define vec2s16_sub(a, b) ((vec2s16){(a).x-(b).x, (a).y-(b).y})
#define vec2s16_dot(a, b) ((a).x * (b).x + (a).y * (b).y)
#define vec2s16_magnitude_sq(a) vec2_dot(a, a)
#define vec2s16_magnitude(a) (sqrtf(vec2_magnitude_sq(a)))
