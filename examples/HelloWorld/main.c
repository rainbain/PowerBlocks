#include "powerblocks/core/system/system.h"

#include "powerblocks/core/graphics/video.h"

#include "powerblocks/core/utils/fonts.h"
#include "powerblocks/core/utils/console.h"

#include <stdio.h>

framebuffer_t frame_buffer __attribute__((aligned(512)));

int main() {
    // Currently hardcoded. Eventually this will need to be
    // got from Starlet using incrprocess communication.
    // Works fine with this for now though.
    video_initialize(VIDEO_MODE_640X480_NTSC_INTERLACED);

    // Fill background with black
    framebuffer_fill_rgba(&frame_buffer, 0x000000FF, vec2i_new(0,0), vec2i_new(VIDEO_WIDTH, VIDEO_HEIGHT));
    console_initialize(&frame_buffer, &fonts_ibm_iso_8x16);

    video_set_framebuffer(&frame_buffer);

    int i = 0;
    
    while(1) {
        video_wait_vsync();

        printf("Hello World! %d\n", i);
        system_flush_dcache(&frame_buffer, sizeof(frame_buffer));
        i++;
    }

    return 0;
}