#include "powerblocks/core/system/system.h"
#include "powerblocks/core/system/ios.h"
#include "powerblocks/core/system/ios_settings.h"

#include "powerblocks/core/graphics/video.h"

#include "powerblocks/core/utils/fonts.h"
#include "powerblocks/core/utils/console.h"

#include <stdio.h>
#include <math.h>

framebuffer_t frame_buffer ALIGN(512);

void retrace_callback() {
    // Make it so we can see the framebuffer changes
    system_flush_dcache(&frame_buffer, sizeof(frame_buffer));
}


int main() {
    // Initialize IOS. Must be done first as many thing use it
    ios_initialize();

    // Get default video mode from IOS and use it to initialize the video interface.
    video_mode_t tv_mode = video_system_default_video_mode();
    video_initialize(tv_mode);

    // Fill background with black
    console_initialize(&frame_buffer, &fonts_ibm_iso_8x16);
    video_set_framebuffer(&frame_buffer);

    // Set the retrace callback to flush the framebuffer before drawing.
    video_set_retrace_callback(retrace_callback);

    // Create Blue Background
    framebuffer_fill_rgba(&frame_buffer, 0x000000FF, vec2i_new(0,0), vec2i_new(VIDEO_WIDTH, VIDEO_HEIGHT));

    // Back Text To Black, Blue Background
    console_set_text_color(0xFFFFFFFF, 0x000000FF);

    // print super cool hello message
    printf("\n\n\n");
    printf("  PowerBlocks SDK\n");
    printf("  Hello World this is a Wii.\n");

    // Create the clock
    while(true) {
        uint64_t total_ms = system_get_time_base_int() / (SYSTEM_TB_CLOCK_HZ / 1000);
        
        int ms = total_ms % 1000;
        int s = (total_ms / 1000) % 60;
        int m = (total_ms / 60000) % 60;
        int h_24 = (total_ms / 3600000) % 24;
        int h_12 = h_24 % 12;
        if(h_12 == 0) h_12 = 12; // Hours are weird

        const char *ampm = (h_24 < 12) ? "AM" : "PM";

        printf("    %02d:%02d:%02d.%03d %s\n",
           h_12, m, s, ms, ampm);
        
        console_set_cursor(
            vec2i_add(console_cursor_position, vec2i_new(0, -console_font->character_size.y))
        );

        // Make it so we can see the framebuffer changes
        system_flush_dcache(&frame_buffer, sizeof(frame_buffer));

        // Wait for vsync
        video_wait_vsync();
    }

    return 0;
}