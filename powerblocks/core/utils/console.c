/**
 * @file console.c
 * @brief Simple console.
 *
 * Provides a simple console for quick debugging.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#include "console.h"

#include <stddef.h>
#include <string.h>

static framebuffer_t* console_framebuffer = NULL;
static vec2i console_cursor_position;
static const framebuffer_font_t* console_font;

void console_initialize(framebuffer_t* framebuffer, const framebuffer_font_t* font) {
    console_framebuffer = framebuffer;
    console_cursor_position = vec2i_new(0, 0);
    console_font = font;
}

void console_set_cursor(vec2i cursor_position) {
    console_cursor_position = cursor_position;
}

void console_put(const char * string) {
    // Break up into lines to feed in
    int line_size = VIDEO_WIDTH / console_font->character_size.x;
    char line[line_size + 1];
    int line_pos = 0;
    vec2i start_pos = console_cursor_position;

    char character;
    while((character = *string++)) {
        // Line break
        if(character == '\n' || (console_cursor_position.x + console_font->character_size.x) > VIDEO_WIDTH) {
            // If the current y takes place past the size, push it back
            int diff = (start_pos.y + console_font->character_size.y) - VIDEO_HEIGHT; 
            if(diff > 0) {
                // Push back frame buffer
                memcpy(console_framebuffer->pixels, console_framebuffer->pixels[diff], sizeof(console_framebuffer->pixels) - diff * VIDEO_WIDTH * 2);
            }

            // Flush current line
            line[line_pos] = 0;
            framebuffer_put_text(console_framebuffer, 0xFFFFFFFF, 0x00000000,
                start_pos, console_font, line);
            
            // Iterate position
            start_pos.x = 0;
            start_pos.y += console_font->character_size.y;

            // Reset counters
            line_pos = 0;
            continue;
        }

        line[line_pos] = character;
        line_pos++;
    }
}