#include "powerblocks/core/system/system.h"
#include "powerblocks/core/system/ios.h"
#include "powerblocks/core/system/ios_settings.h"

#include "powerblocks/core/graphics/video.h"

#include "powerblocks/core/utils/fonts.h"
#include "powerblocks/core/utils/console.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdalign.h>

framebuffer_t frame_buffer __attribute__((aligned(512)));

int main() {
    // Initialize IOS. Must be done first as many thing use it
    ios_initialize();

    // Get default video mode from IOS and use it to initialize the video interface.
    video_initialize(video_system_default_video_mode());

    // Fill background with black
    framebuffer_fill_rgba(&frame_buffer, 0x000000FF, vec2i_new(0,0), vec2i_new(VIDEO_WIDTH, VIDEO_HEIGHT));
    console_initialize(&frame_buffer, &fonts_ibm_iso_8x16);
    video_set_framebuffer(&frame_buffer);
    
    int i = 0;
    while(1) {
        printf("Hello World: %d\n", i++);
        vTaskDelay(500 / portTICK_PERIOD_MS);

        video_wait_vsync();
        system_flush_dcache(&frame_buffer, sizeof(frame_buffer));
    }

    return 0;
}