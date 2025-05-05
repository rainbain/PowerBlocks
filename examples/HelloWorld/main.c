#include "powerblocks/core/system/system.h"

#include "powerblocks/core/graphics/video.h"

#include "powerblocks/core/utils/fonts.h"

framebuffer_t frame_buffer __attribute__((aligned(32)));

int main() {
    // Currently hardcoded. Eventually this will need to be
    // got from Starlet using incrprocess communication.
    // Works fine with this for now though.
    video_initialize(VIDEO_MODE_640X480_NTSC_INTERLACED);

    // Fill with cool and fun background pattern.
    for(int y = 0; y < VIDEO_HEIGHT; y++) {
        for(int x = 0; x < VIDEO_WIDTH; x++) {
            frame_buffer.pixels[y][x] = x;
        }
    }

    video_set_framebuffer(&frame_buffer);

    framebuffer_put_text(&frame_buffer, 0xFFFFFFFF, 0x000000A0, vec2i_new(100, 100), &fonts_ibm_iso_8x16, "Hello World from PowerBlocks");

    system_flush_dcache_nosync(&frame_buffer, sizeof(frame_buffer));

    while(1) {
        system_delay_int(SYSTEM_S_TO_TICKS(1.0 / 30.0));
    }

    return 0;
}