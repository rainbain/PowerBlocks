/**
 * @file framebuffer.c
 * @brief Functions for managing and rendering framebuffers.
 *
 * Able to fill colors, copy data to and from, as well as render
 * text into frame buffers.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "framebuffer.h"
 
//
// A few of the following functions involving basic frame buffer operations
// are written using ChatGPT. I just really did not want to get tied up in a few
// hours of redundant YUY color operations when I just wanted to add support
// for alpha blending.
//
// I apologize for their length.
//

static inline uint8_t clamp(int val) {
    return (uint8_t)((val < 0) ? 0 : (val > 255 ? 255 : val));
}

void framebuffer_copy_rgba_into(framebuffer_t* framebuffer, const uint32_t *rgba, vec2i position, vec2i size) {
    // General protection functions.
    // Its good to keep these safe so that future operations can
    // use them willy-nilly.
    if(position.x > VIDEO_WIDTH)
        position.x = VIDEO_WIDTH;
    if(position.x < 0)
        position.x = 0;
    
    if(position.y > VIDEO_HEIGHT)
        position.y = VIDEO_HEIGHT;
    if(position.y < 0)
        position.y = 0;
    
    if(size.x < 0)
        size.x = 0;
    
    if(size.y < 0)
        size.y = 0;
    
    if(size.x + position.x >= VIDEO_WIDTH)
        size.x = VIDEO_WIDTH - position.x;
    
    if(size.y + position.y >= VIDEO_HEIGHT)
        size.y = VIDEO_HEIGHT - position.y;

    for (int y = 0; y < size.y; y++) {
        int x = 0;
        for (; x + 1 < size.x; x += 2) {
            // --- Source pixels (RGBA assumed as 0xRRGGBBAA) ---
            uint32_t src0 = rgba[x + y * size.x];
            uint32_t src1 = rgba[(x + 1) + y * size.x];
            uint8_t r0 = (src0 >> 24) & 0xFF;
            uint8_t g0 = (src0 >> 16) & 0xFF;
            uint8_t b0 = (src0 >> 8)  & 0xFF;
            uint8_t a0 =  src0        & 0xFF;
            
            uint8_t r1 = (src1 >> 24) & 0xFF;
            uint8_t g1 = (src1 >> 16) & 0xFF;
            uint8_t b1 = (src1 >> 8)  & 0xFF;
            uint8_t a1 =  src1        & 0xFF;

            // --- Destination (background) pixels ---
            // For YUYV, even pixel stores: high byte = Y0, low byte = shared U.
            // Odd pixel stores: high byte = Y1, low byte = shared V.
            uint16_t dest_even = framebuffer->pixels[y + position.y][x + position.x];
            uint16_t dest_odd  = framebuffer->pixels[y + position.y][(x + 1) + position.x];
            uint8_t Y0 = (dest_even >> 8) & 0xFF;
            uint8_t U_shared = dest_even & 0xFF;
            uint8_t Y1 = (dest_odd  >> 8) & 0xFF;
            uint8_t V_shared = dest_odd & 0xFF;
            // For both pixels, use the same U and V from the pair.
            
            // --- Convert background YUV to RGB ---
            // Standard YUV to RGB conversion:
            //   C = Y - 16, D = U - 128, E = V - 128,
            //   R = clamp((298 * C + 409 * E + 128) >> 8)
            //   G = clamp((298 * C - 100 * D - 208 * E + 128) >> 8)
            //   B = clamp((298 * C + 516 * D + 128) >> 8)
            int C0 = Y0 - 16;
            int C1 = Y1 - 16;
            int D = U_shared - 128;  // same for both pixels
            int E = V_shared - 128;  // same for both pixels
            uint8_t r_bg0 = clamp((298 * C0 + 409 * E + 128) >> 8);
            uint8_t g_bg0 = clamp((298 * C0 - 100 * D - 208 * E + 128) >> 8);
            uint8_t b_bg0 = clamp((298 * C0 + 516 * D + 128) >> 8);
            uint8_t r_bg1 = clamp((298 * C1 + 409 * E + 128) >> 8);
            uint8_t g_bg1 = clamp((298 * C1 - 100 * D - 208 * E + 128) >> 8);
            uint8_t b_bg1 = clamp((298 * C1 + 516 * D + 128) >> 8);

            // --- Blend source with background ---
            // Blending formula: out = (src * alpha + bg * (255 - alpha)) / 255
            uint8_t r_out0 = (uint8_t)((r0 * a0 + r_bg0 * (255 - a0)) / 255);
            uint8_t g_out0 = (uint8_t)((g0 * a0 + g_bg0 * (255 - a0)) / 255);
            uint8_t b_out0 = (uint8_t)((b0 * a0 + b_bg0 * (255 - a0)) / 255);
            uint8_t r_out1 = (uint8_t)((r1 * a1 + r_bg1 * (255 - a1)) / 255);
            uint8_t g_out1 = (uint8_t)((g1 * a1 + g_bg1 * (255 - a1)) / 255);
            uint8_t b_out1 = (uint8_t)((b1 * a1 + b_bg1 * (255 - a1)) / 255);

            // --- Convert blended RGB to YUV ---
            // Using conversion formulas (the same as in your original code):
            uint8_t y0_new = (uint8_t)((66 * r_out0 + 129 * g_out0 + 25 * b_out0 + 128) >> 8) + 16;
            uint8_t u0_new = (uint8_t)((-38 * r_out0 - 74 * g_out0 + 112 * b_out0 + 128) >> 8) + 128;
            uint8_t v0_new = (uint8_t)((112 * r_out0 - 94 * g_out0 - 18 * b_out0 + 128) >> 8) + 128;
            uint8_t y1_new = (uint8_t)((66 * r_out1 + 129 * g_out1 + 25 * b_out1 + 128) >> 8) + 16;
            uint8_t u1_new = (uint8_t)((-38 * r_out1 - 74 * g_out1 + 112 * b_out1 + 128) >> 8) + 128;
            uint8_t v1_new = (uint8_t)((112 * r_out1 - 94 * g_out1 - 18 * b_out1 + 128) >> 8) + 128;

            // For the final YUYV pair, average U and V from the two blended pixels.
            uint8_t u_avg = (u0_new + u1_new) / 2;
            uint8_t v_avg = (v0_new + v1_new) / 2;

            // --- Write the blended, converted pixels back to the framebuffer ---
            // Even pixel: high byte = new Y0, low byte = averaged U
            framebuffer->pixels[y + position.y][x + position.x] =
                (y0_new << 8) | u_avg;
            // Odd pixel: high byte = new Y1, low byte = averaged V
            framebuffer->pixels[y + position.y][(x + 1) + position.x] =
                (y1_new << 8) | v_avg;
        }

        // --- Handle odd-width image (one pixel left on the row) ---
        if (size.x & 1) {
            uint32_t src = rgba[x + y * size.x];
            uint8_t r = (src >> 24) & 0xFF;
            uint8_t g = (src >> 16) & 0xFF;
            uint8_t b = (src >> 8)  & 0xFF;
            uint8_t a =  src        & 0xFF;

            // For a solitary pixel we only have one Y and one shared chroma.
            uint16_t dest = framebuffer->pixels[y + position.y][x + position.x];
            uint8_t Y = (dest >> 8) & 0xFF;
            uint8_t U = dest & 0xFF; // we use U for blending (or could choose V)
            // For our blending, we’ll use U for both chroma channels.
            int C = Y - 16;
            int D = U - 128;
            int E = U - 128; // approximate V using U
            uint8_t r_bg = clamp((298 * C + 409 * E + 128) >> 8);
            uint8_t g_bg = clamp((298 * C - 100 * D - 208 * E + 128) >> 8);
            uint8_t b_bg = clamp((298 * C + 516 * D + 128) >> 8);

            uint8_t r_out = (uint8_t)((r * a + r_bg * (255 - a)) / 255);
            uint8_t g_out = (uint8_t)((g * a + g_bg * (255 - a)) / 255);
            uint8_t b_out = (uint8_t)((b * a + b_bg * (255 - a)) / 255);

            uint8_t y_new = (uint8_t)((66 * r_out + 129 * g_out + 25 * b_out + 128) >> 8) + 16;
            uint8_t u_new = (uint8_t)((-38 * r_out - 74 * g_out + 112 * b_out + 128) >> 8) + 128;
            framebuffer->pixels[y + position.y][x + position.x] = (y_new << 8) | u_new;
        }
    }
}

void framebuffer_fill_rgba(framebuffer_t* framebuffer, uint32_t rgba, vec2i a, vec2i b) {
    // Extract fill color components from 0xRRGGBBAA
    uint8_t r_fill = (rgba >> 24) & 0xFF;
    uint8_t g_fill = (rgba >> 16) & 0xFF;
    uint8_t b_fill = (rgba >> 8)  & 0xFF;
    uint8_t a_fill = rgba & 0xFF;

    if(a.x < 0)
        a.x = 0;
    if(a.x > VIDEO_WIDTH)
        a.x = VIDEO_WIDTH;
    if(a.y < 0)
        a.y = 0;
    if(a.y > VIDEO_HEIGHT)
        a.y = VIDEO_HEIGHT;
    if(b.x < 0)
        b.x = 0;
    if(b.x > VIDEO_WIDTH)
        b.x = VIDEO_WIDTH;
    if(b.y < 0)
        b.y = 0;
    if(b.y > VIDEO_HEIGHT)
        b.y = VIDEO_HEIGHT;
    
    // Calculate region dimensions. Here we assume b is exclusive.
    int width  = b.x - a.x;
    int height = b.y - a.y;
    
    // Iterate over every row in the region.
    for (int y = a.y; y < a.y + height; y++) {
        int x;
        // Process pixels in pairs.
        for (x = a.x; x + 1 < a.x + width; x += 2) {
            // Fetch the two YUYV pixels.
            uint16_t dest_even = framebuffer->pixels[y][x];
            uint16_t dest_odd  = framebuffer->pixels[y][x + 1];
            
            // Extract background Y (luma) and chroma from the even (U) and odd (V) pixels.
            uint8_t Y0 = (dest_even >> 8) & 0xFF;
            uint8_t U_shared = dest_even & 0xFF;
            uint8_t Y1 = (dest_odd >> 8) & 0xFF;
            uint8_t V_shared = dest_odd & 0xFF;
            
            // Convert the background YUYV to RGB.
            // For both pixels, use the same U/V pair.
            int C0 = Y0 - 16;
            int C1 = Y1 - 16;
            int D = U_shared - 128;
            int E = V_shared - 128;
            uint8_t r_bg0 = clamp((298 * C0 + 409 * E + 128) >> 8);
            uint8_t g_bg0 = clamp((298 * C0 - 100 * D - 208 * E + 128) >> 8);
            uint8_t b_bg0 = clamp((298 * C0 + 516 * D + 128) >> 8);
            uint8_t r_bg1 = clamp((298 * C1 + 409 * E + 128) >> 8);
            uint8_t g_bg1 = clamp((298 * C1 - 100 * D - 208 * E + 128) >> 8);
            uint8_t b_bg1 = clamp((298 * C1 + 516 * D + 128) >> 8);
            
            // Alpha blend the fill color with the background for each pixel.
            uint8_t r_out0 = (uint8_t)((r_fill * a_fill + r_bg0 * (255 - a_fill)) / 255);
            uint8_t g_out0 = (uint8_t)((g_fill * a_fill + g_bg0 * (255 - a_fill)) / 255);
            uint8_t b_out0 = (uint8_t)((b_fill * a_fill + b_bg0 * (255 - a_fill)) / 255);
            
            uint8_t r_out1 = (uint8_t)((r_fill * a_fill + r_bg1 * (255 - a_fill)) / 255);
            uint8_t g_out1 = (uint8_t)((g_fill * a_fill + g_bg1 * (255 - a_fill)) / 255);
            uint8_t b_out1 = (uint8_t)((b_fill * a_fill + b_bg1 * (255 - a_fill)) / 255);
            
            // Convert the blended RGB values back to YUV using the same formulas.
            uint8_t y0_new = (uint8_t)((66 * r_out0 + 129 * g_out0 + 25 * b_out0 + 128) >> 8) + 16;
            uint8_t u0_new = (uint8_t)((-38 * r_out0 - 74 * g_out0 + 112 * b_out0 + 128) >> 8) + 128;
            uint8_t v0_new = (uint8_t)((112 * r_out0 - 94 * g_out0 - 18 * b_out0 + 128) >> 8) + 128;
            
            uint8_t y1_new = (uint8_t)((66 * r_out1 + 129 * g_out1 + 25 * b_out1 + 128) >> 8) + 16;
            uint8_t u1_new = (uint8_t)((-38 * r_out1 - 74 * g_out1 + 112 * b_out1 + 128) >> 8) + 128;
            uint8_t v1_new = (uint8_t)((112 * r_out1 - 94 * g_out1 - 18 * b_out1 + 128) >> 8) + 128;
            
            // Average the U and V values from the two pixels for the shared chroma.
            uint8_t u_avg = (u0_new + u1_new) / 2;
            uint8_t v_avg = (v0_new + v1_new) / 2;
            
            // Write the resulting YUYV pixels back into the framebuffer.
            // Even pixel: high byte = new Y, low byte = averaged U.
            framebuffer->pixels[y][x] = (y0_new << 8) | u_avg;
            // Odd pixel: high byte = new Y, low byte = averaged V.
            framebuffer->pixels[y][x + 1] = (y1_new << 8) | v_avg;
        }
        
        // If the width of the fill zone is odd, handle the final (rightmost) pixel.
        if ((a.x + width) & 1) {
            // Process the last pixel at column (a.x + width - 1)
            int final_x = a.x + width - 1;
            uint16_t dest = framebuffer->pixels[y][final_x];
            uint8_t Y = (dest >> 8) & 0xFF;
            uint8_t U = dest & 0xFF; // Using U (or V) for blending
            int C = Y - 16;
            int D = U - 128;
            int E = U - 128; // Approximating V using U
            uint8_t r_bg = clamp((298 * C + 409 * E + 128) >> 8);
            uint8_t g_bg = clamp((298 * C - 100 * D - 208 * E + 128) >> 8);
            uint8_t b_bg = clamp((298 * C + 516 * D + 128) >> 8);
            
            uint8_t r_out = (uint8_t)((r_fill * a_fill + r_bg * (255 - a_fill)) / 255);
            uint8_t g_out = (uint8_t)((g_fill * a_fill + g_bg * (255 - a_fill)) / 255);
            uint8_t b_out = (uint8_t)((b_fill * a_fill + b_bg * (255 - a_fill)) / 255);
            
            uint8_t y_new = (uint8_t)((66 * r_out + 129 * g_out + 25 * b_out + 128) >> 8) + 16;
            uint8_t u_new = (uint8_t)((-38 * r_out - 74 * g_out + 112 * b_out + 128) >> 8) + 128;
            
            // Write back the single pixel.
            framebuffer->pixels[y][final_x] = (y_new << 8) | u_new;
        }
    }
}

void framebuffer_put_text(framebuffer_t* framebuffer, uint32_t foreground, uint32_t background, vec2i position, const framebuffer_font_t* font, const char* str) {
    // Lets get this into 2 ints.
    vec2i character_size = vec2i_new(font->character_size.x, font->character_size.y);

    // First start with a glyph containing the color data of one character.
    uint32_t glyph[character_size.y][character_size.x];

    // Number of bytes in a character.
    uint32_t character_bytes = (character_size.x * character_size.y) / 8;

    while(*str != 0) {
        // Look up character in font
        const uint8_t* character_data = font->font_data + character_bytes * ((uint32_t)*str);

        // Render glyph
        for(int y = 0; y < character_size.y; y++) {
            for(int x = 0; x < character_size.x;) {
                uint8_t v = *character_data++;
                for(int bit = 0x80; bit > 0; bit >>= 1) {
                    glyph[y][x] = (v & bit) ? foreground : background;

                    x++;
                }
            }
        }

        framebuffer_copy_rgba_into(framebuffer, (const uint32_t*)glyph, position, character_size);

        position.x += character_size.x;

        str++;
    }
}