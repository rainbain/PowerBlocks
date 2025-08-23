/**
 * @file console.h
 * @brief Simple console.
 *
 * Provides a simple console for quick debugging.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

#include "powerblocks/core/graphics/framebuffer.h"

// This code was written while I was on a small
// airplane. May have weird formatting

extern vec2i console_cursor_position;
extern uint32_t console_foreground_color;
extern uint32_t console_background_color;
extern const framebuffer_font_t* console_font;

 /**
 * @brief Initializes the visual console.
 *
 * Attaches to a frame buffer. Will write directly into it when displaying
 * text.
 * 
 * Sets stdout into the console.
 * 
 * Sets console position back to 0,0
 * 
 * @param framebuffer Framebuffer to attach to
 */
extern void console_initialize(framebuffer_t* framebuffer, const framebuffer_font_t* font);

 /**
 * @brief Sets the cursor position of the console
 *
 * Set the position of the cursor in pixels.
 */
extern void console_set_cursor(vec2i cursor_position);

/**
 * @brief Sets the color text color.
 * 
 * Sets the color text color.
 */
extern void console_set_text_color(uint32_t foreground, uint32_t background);

 /**
 * @brief Puts a string into the console.
 *
 * Prints a string into the console. Will update cursor positions.asm
 * '\n' will generate a new line.
 */
extern void console_put(const char * string);