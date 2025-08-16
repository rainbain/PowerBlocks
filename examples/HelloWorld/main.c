#include "powerblocks/core/system/system.h"
#include "powerblocks/core/system/ios.h"
#include "powerblocks/core/system/ios_settings.h"

#include "powerblocks/core/graphics/video.h"
#include "powerblocks/core/graphics/gx.h"

#include "powerblocks/core/utils/fonts.h"
#include "powerblocks/core/utils/console.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <math.h>

framebuffer_t frame_buffer __attribute__((aligned(512)));
uint8_t fifo_buffer[4096] __attribute__((aligned(32)));

#define WINDOW_SIZE 320

uint32_t window[WINDOW_SIZE][WINDOW_SIZE];
bool ready_for_copy = false;

static void copy_framebuffers();

int main() {
    // Initialize IOS. Must be done first as many thing use it
    ios_initialize();

    // Get default video mode from IOS and use it to initialize the video interface.
    video_mode_t tv_mode = video_system_default_video_mode();
    video_initialize(tv_mode);
    video_set_retrace_callback(copy_framebuffers);

    // Fill background with black
    console_initialize(&frame_buffer, &fonts_ibm_iso_8x16);
    video_set_framebuffer(&frame_buffer);

    const video_profile_t* profile = video_get_profile(tv_mode);

    gx_fifo_t fifo;
    gx_fifo_create(&fifo, fifo_buffer, sizeof(fifo_buffer));
    gx_initialize(&fifo, profile);

    matrix4 projection;
    // TODO: Get aspect ratio
    matrix4_perspective(projection, 60.0f / 180.0f * M_PI, 4.0f / 3.0f, 10.0f, 300.0f);
    //matrix4_orthographic(projection, -50.0f, 50.0f, -50.0f, 50.0f, -50.0f, 50.0f);
    gx_set_projection(projection, true);

    gx_vtxdesc_set(GX_VTXDESC_POSITION, GX_VTXATTR_DATA_DIRECT);
    gx_vtxdesc_set(GX_VTXDESC_COLOR0, GX_VTXATTR_DATA_DIRECT);

    gx_vtxfmtattr_set(0, GX_VTXDESC_POSITION, GX_VTXATTR_POS_XYZ, GX_VTXATTR_F32, 0);
    gx_vtxfmtattr_set(0, GX_VTXDESC_COLOR0, GX_VTXATTR_RGBA, GX_VTXATTR_RGBA8, 0);


    gx_set_color_channels(1);

    gx_flush();

    printf("Awaiting first copy...\n");

    float t = 0.0f;
    while(1) {
        matrix34 rotation;
        matrix34 translation;
        matrix34 model;
        matrix34_rotate_y(rotation, t);
        vec3 ts = {0.0f, 0.0f, -30.0f};
        matrix34_translation(translation, &ts);
        matrix34_multiply(model, translation, rotation);

        gx_load_psn_matrix(model, GX_PSNMTX_0);
        gx_set_psn_matrix(GX_PSNMTX_0);

        gx_begin(GX_TRIANGLES, 0, 3);
            GX_WPAR_F32 = 0.0f;
            GX_WPAR_F32 = 0.0f;
            GX_WPAR_F32 = 00.0f;
            GX_WPAR_U32 = 0xFF0000FF;
            GX_WPAR_F32 = 10.0f;
            GX_WPAR_F32 = 0.0f;
            GX_WPAR_F32 = 00.0f;
            GX_WPAR_U32 = 0x00FF00FF;
            GX_WPAR_F32 = 0.0f;
            GX_WPAR_F32 = 10.0f;
            GX_WPAR_F32 = 0.0f;
            GX_WPAR_U32 = 0x0000FFFF;
        gx_end();

        t += M_PI * 2.0f / 60.0f;

        gx_draw_done();

        ready_for_copy = true;

        video_wait_vsync();
    }

    return 0;
}

void copy_framebuffers() {
    if(ready_for_copy) {
        ready_for_copy = false;

        gx_copy_framebuffer(&frame_buffer, true);
        gx_flush();
    }
}