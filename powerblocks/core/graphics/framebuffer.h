/**
 * @file framebuffer.h
 * @brief Functions for managing and rendering framebuffers.
 *
 * Able to fill colors, copy data to and from, as well as render
 * text into frame buffers.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */
#pragma once

#include <stdint.h>

#include "powerblocks/core/utils/math/vec2.h"

// Defines for some standards used across the video
#define VIDEO_WIDTH  640
#define VIDEO_HEIGHT 480

/**
 * @struct framebuffer_t
 * @brief Data format for framebuffers.
 *
 * 640x480 pixel YUY 16 bit values.
 */
typedef struct {
    uint16_t pixels[VIDEO_HEIGHT][VIDEO_WIDTH];
} framebuffer_t;

/**
 * @struct framebuffer_font_t
 * @brief Used to describe a font for the framebuffer to use.
 *
 * Contains the 1 bit per pixel font data and the font size;
 * Expects a VGA bitmap font.
 * 
 * Smallest supported size is 8x8
 * Size must be a multiple of 8
 */
typedef struct {
    const uint8_t* font_data;
    vec2s16 character_size;
} framebuffer_font_t;

/**
 * @brief Copys RGBA8888 data into a frame buffer.
 *
 * Copys data into a position of the framebuffer.
 * Converts into the YUY data of the framebuffer.
 * 
 * Blends as it goes. Very nice
 * 
 * @param framebuffer Framebuffer to render into.
 * @param rgba RGBA8888 color data of width x height.
 * @param x_pos x position in the frame buffer.
 * @param y_pos y position in the frame buffer.
 */
extern void framebuffer_copy_rgba_into(framebuffer_t* framebuffer, const uint32_t *rgba, vec2i position, vec2i size);

/**
 * @brief Fills a section of the frame buffer with RGBA8888 data.
 *
 * Fills the frame buffer with a color while doing
 * blending as needed
 * 
 * @param framebuffer Framebuffer to render into.
 * @param rgba RGBA8888 color
 * @param a - Start position of the fill
 * @param b - Stop position of the fill
 */
extern void framebuffer_fill_rgba(framebuffer_t* framebuffer, uint32_t rgba, vec2i a, vec2i b);

/**
 * @brief Renders a bitmap font to the framebuffer.
 *
 * Renders a bitmap font to the frame buffer using the specified font.
 * Special characters are not supported. It will just render that slot in the font.
 * 
 * @param framebuffer Framebuffer to render into.
 * @param foreground RGBA8888 Color of the text itself
 * @param background RGBA8888 Color of the background
 * @param position Pixel position of the top left corner of the text.
 * @param font Font to use
 * @param str Zero terminated string to render
 */
extern void framebuffer_put_text(framebuffer_t* framebuffer, uint32_t foreground, uint32_t background, vec2i position, const framebuffer_font_t* font, const char* str);