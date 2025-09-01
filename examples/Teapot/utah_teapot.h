/**
 * @file utah-teapot.h
 * @brief Asset of the utah teapot.
 *
 * Asset of the utah teapot.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>

#include "powerblocks/core/utils/math/matrix34.h"

// I opted to store this as a header
// With fixed point int16s.
// So we get to mess around with vertex formats a lil bit uwu.

// These are all 32 byte aligned.
extern const int16_t utah_teapot_model_positions[];
extern const int16_t utah_teapot_model_texcoords[];
extern const int16_t utah_teapot_model_normals[];
extern const uint16_t utah_teapot_model_indices[];
extern const uint16_t utah_teapot_model_indices_count;
extern const float utah_teapot_model_texcoords_scale;

// Texture Data
extern const uint16_t utah_teapot_texture_width;
extern const uint16_t utah_teapot_texture_height;
extern const uint8_t utah_teapot_texture_data[];

extern void utah_teapot_draw();