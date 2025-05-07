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
#include <stdbool.h>
#include <stdio.h>

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

void console_put(const char* string) {
    if(console_framebuffer == NULL)
        return;

    // Buffer to hold line
    const int line_size = VIDEO_WIDTH / console_font->character_size.x;
    char line[line_size + 1];
    int line_index = 0;
    vec2i line_pos = console_cursor_position;

    char character;
    do {
        character = *string++;

        if(character != 0 && character != '\n') {
            // Accumulate into buffer
            line[line_index] = character;
            line_index++;

            console_cursor_position.x += console_font->character_size.x;
        }
        
        // Generate new line if needed
        bool new_line = (character == '\n') || ((console_cursor_position.x + console_font->character_size.x) > VIDEO_WIDTH);
        if(new_line) {
            console_cursor_position.x = 0;
            console_cursor_position.y += console_font->character_size.y;

            // Shift console up
            int diff = (console_cursor_position.y + console_font->character_size.y) - VIDEO_HEIGHT; 
            if(diff > 0) {
                memcpy(console_framebuffer->pixels, console_framebuffer->pixels[diff], sizeof(console_framebuffer->pixels) - diff * VIDEO_WIDTH * sizeof(uint16_t));

                console_cursor_position.y -= diff;
                line_pos.y -= diff;

                // Clear new space
                framebuffer_fill_rgba(console_framebuffer, 0x000000FF, vec2i_new(0,VIDEO_HEIGHT-diff), vec2i_new(VIDEO_WIDTH, VIDEO_HEIGHT));
            }
        }

        // Flush console as needed
        bool flush_console = new_line || (character == 0);
        if(flush_console) {
            line[line_index] = 0;

            framebuffer_put_text(console_framebuffer, 0xFFFFFFFF, 0x000000FF,
                line_pos, console_font, line);
            
            line_pos = console_cursor_position;
            line_index = 0;
        }

    } while(character != 0);
}
/*
void console_put(const char * string) {
    // Break up into lines to feed in
    int line_size = VIDEO_WIDTH / console_font->character_size.x;
    char line[line_size + 1];
    int line_pos = 0;
    vec2i start_pos = console_cursor_position;

    char character;
    while(true) {
        console_cursor_position.x += console_font->character_size.x;

        character = *string++;

        // Line break
        bool new_line = character == '\n' || console_cursor_position.x > VIDEO_WIDTH;
        if(new_line || character == 0) {
            // If the current y takes place past the size, push it back
            int diff = (start_pos.y + console_font->character_size.y) - VIDEO_HEIGHT; 
            if(diff > 0) {
                // Push back frame buffer
                memcpy(console_framebuffer->pixels, console_framebuffer->pixels[diff], sizeof(console_framebuffer->pixels) - diff * VIDEO_WIDTH * sizeof(uint16_t));

                console_cursor_position.y = VIDEO_HEIGHT - console_font->character_size.y;

                // Clear new space
                framebuffer_fill_rgba(console_framebuffer, colors[color_index & 7], vec2i_new(0,VIDEO_HEIGHT-diff), vec2i_new(VIDEO_WIDTH, VIDEO_HEIGHT));
                color_index++;
            }

            // Flush current line
            line[line_pos] = 0;
            framebuffer_put_text(console_framebuffer, 0xFFFFFFFF, 0x000000FF,
                start_pos, console_font, line);
            
            // Iterate position
            if(new_line) {
                console_cursor_position.x = 0;
                console_cursor_position.y += console_font->character_size.y;
                line_pos = 0;
            }

            start_pos = console_cursor_position;

            if(character == 0)
                break;
        }

        if(character != '\n') {
            line[line_pos] = character;
            line_pos++;
        }
    }
}
*/