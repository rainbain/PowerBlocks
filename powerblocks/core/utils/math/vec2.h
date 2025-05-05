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