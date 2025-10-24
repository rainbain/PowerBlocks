#include "powerblocks/core/system/system.h"
#include "powerblocks/core/system/gpio.h"
#include "powerblocks/core/ios/ios.h"
#include "powerblocks/core/ios/ios_settings.h"

#include "powerblocks/core/graphics/video.h"

#include "powerblocks/core/utils/fonts.h"
#include "powerblocks/core/utils/console.h"

#include "powerblocks/core/bluetooth/bltootls.h"

#include "powerblocks/input/wiimote/wiimote.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
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
    video_set_retrace_callback(retrace_callback);

    // Create Blue Background
    framebuffer_fill_rgba(&frame_buffer, 0x000000FF, vec2i_new(0,0), vec2i_new(VIDEO_WIDTH, VIDEO_HEIGHT));

    // Back Text To Black, Blue Background
    console_set_text_color(0xFFFFFFFF, 0x000000FF);

    // print super cool hello message
    printf("\n\n\n");
    printf("  PowerBlocks SDK\n");
    printf("  Hello World this is a Wii.\n");

    int ret = bltools_initialize();
    if(ret < 0) {
        printf("Failed To Init Bluetooth.\n");
    }

    gpio_set_direction(GPIO_SENSOR_BAR, true);
    wiimotes_initialize();

    //ret = bltools_begin_automatic_discovery(portMAX_DELAY);
    //if(ret < 0) {
    //    printf("Auto discovery failed.\n");
    //}

    while(!WIIMOTES[0].driver) {

    }

    printf("Done!\n");

    ret = wiimote_set_reporting(&WIIMOTES[0], WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_IR);
    if(ret < 0) {
        printf("Failed to find a reporting mode!\n");
    }



    // Create the clock
    int meows = 0;
    while(true) {
        wiimote_poll();

        vec2i cursor = WIIMOTES[0].cursor.pos;

        if(WIIMOTES[0].driver) {
            if(WIIMOTES[0].buttons.down & WIIMOTE_BUTTONS_A) {
                printf("A %d, CX: %d, CY: %d\n", meows, cursor.x, cursor.y);
                meows++;
            }
        }

        vec2i size = vec2i_new(5, 5);
        framebuffer_fill_rgba(&frame_buffer, 0xFF0000FF, vec2i_sub(cursor, size), vec2i_add(cursor, size));

        // Wait for vsync
        video_wait_vsync();
    }

    return 0;
}