/**
 * @file main.c
 * @brief Main file for the teapot (GX Demo)
 *
 * Main file for the teapot (GX) demo.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */
#include <math.h>

#include "powerblocks/core/system/system.h"
#include "powerblocks/core/ios/ios.h"

#include "powerblocks/core/graphics/video.h"
#include "powerblocks/core/graphics/gx.h"

#include "powerblocks/core/utils/math/matrix3.h"
#include "powerblocks/core/utils/math/matrix4.h"
#include "powerblocks/core/utils/math/matrix34.h"

#include "utah_teapot.h"

framebuffer_t frame_buffer ALIGN(512);
uint8_t fifo_buffer[GX_FIFO_MINIMUM_SIZE] ALIGN(32);

// Set to true so that on the next vblank
// period we copy the new frame buffer
bool framebuffer_ready;

static void copy_framebuffer();
static void game_loop();

static void setup_scene() {
    // Ooo purple background
    gx_set_clear_color(7, 0, 28, 255);

    // Camera setup
    matrix4 projection;
    matrix4_perspective(projection, 60.0f / 180.0f * M_PI, 4.0f / 3.0f, 0.1f, 100.0f);
    gx_flash_projection(projection, true);
}

static void scene_light_0() {
    gx_light_t light;
    light.color = 0xFFFFFFFF;

    light.position = vec3_new(0, 5.0, 2.0);
    light.direction = vec3_new(0, 1.0, 0.2);
    vec3_normalize(&light.direction);

    light.cos_attenuation = vec3_new(1.0, 0.0, 0.0);
    light.distance_attenuation = vec3_new(1.0, 0.0, 0.0);

    gx_flash_light(GX_LIGHT_ID_0, &light);
}

static void scene_light_1(float time) {
    gx_light_t light;
    light.color = 0xFF0000FF;

    vec3 target = vec3_new(sin(time * 2.142f) * 0.5 - 0.25, cos(time * 3.142f) * 0.5 -0.75f, -2.0);

    light.position = vec3_new(0, 0, 2.0);

    target = vec3_to(light.position, target);
    target = vec3_muls(target, -1.0); // hardware direction negative
    light.direction = target;


    float cutoff = 10.0 * M_PI / 180.0f;
    float cr = cosf(cutoff);
    light.cos_attenuation = vec3_new(-cr/(1.0f-cr), 0.0f, 1.0f/(1.0f-cr));
    light.distance_attenuation = vec3_new(1.0f, 0.0, 0.0);
    gx_flash_light(GX_LIGHT_ID_1, &light);
}

static void scene_light_2(float time) {
    gx_light_t light;
    light.color = 0x00FF00FF;

    vec3 target = vec3_new(sin(time * 2.439f) * 0.5 - 0.25, cos(time * 3.9982f) * 0.5 -0.75f, -2.0);

    light.position = vec3_new(-1.0f, 1.0f, 1.0f);

    target = vec3_to(light.position, target);
    target = vec3_muls(target, -1.0f); // hardware direction negative
    light.direction = target;


    float cutoff = 10.0 * M_PI / 180.0f;
    float cr = cosf(cutoff);
    light.cos_attenuation = vec3_new(-cr/(1.0f-cr), 0.0f, 1.0f/(1.0f-cr));
    light.distance_attenuation = vec3_new(1.0f, 0.0, 0.0);
    gx_flash_light(GX_LIGHT_ID_2, &light);
}

static void scene_light_3(float time) {
    gx_light_t light;
    light.color = 0x0000FFFF;

    vec3 target = vec3_new(sin(time * 3.3212f) * 0.5 - 0.25, cos(time * 2.92912f) * 0.5 -0.75f, -2.0);

    light.position = vec3_new(1.0f, 1.0f, 1.0);

    target = vec3_to(light.position, target);
    target = vec3_muls(target, -1.0); // hardware direction negative
    light.direction = target;


    float cutoff = 10.0 * M_PI / 180.0f;
    float cr = cosf(cutoff);
    light.cos_attenuation = vec3_new(-cr/(1.0f-cr), 0.0f, 1.0f/(1.0f-cr));
    light.distance_attenuation = vec3_new(1.0f, 0.0, 0.0);
    gx_flash_light(GX_LIGHT_ID_3, &light);
}

static void scene_light_4(float time) {
    gx_light_t light;
    light.color = 0xFF00FFFF;

    vec3 target = vec3_new(sin(time * 2.2191) * 0.5 - 0.25, cos(time * 4.58275) * 0.5 -0.75f, -2.0);

    light.position = vec3_new(0, 0, 2.0);

    target = vec3_to(light.position, target);
    target = vec3_muls(target, -1.0); // hardware direction negative
    light.direction = target;


    float cutoff = 10.0 * M_PI / 180.0f;
    float cr = cosf(cutoff);
    light.cos_attenuation = vec3_new(-cr/(1.0f-cr), 0.0f, 1.0f/(1.0f-cr));
    light.distance_attenuation = vec3_new(1.0f, 0.0, 0.0);
    gx_flash_light(GX_LIGHT_ID_4, &light);
}

static void scene_light_5(float time) {
    gx_light_t light;
    light.color = 0x00FFFFFF;

    vec3 target = vec3_new(sin(time * 4.1319f) * 0.5 - 0.25, cos(time * 4.4929) * 0.5 -0.75f, -2.0);

    light.position = vec3_new(-1.0f, 1.0f, 1.0f);

    target = vec3_to(light.position, target);
    target = vec3_muls(target, -1.0f); // hardware direction negative
    light.direction = target;


    float cutoff = 10.0 * M_PI / 180.0f;
    float cr = cosf(cutoff);
    light.cos_attenuation = vec3_new(-cr/(1.0f-cr), 0.0f, 1.0f/(1.0f-cr));
    light.distance_attenuation = vec3_new(1.0f, 0.0, 0.0);
    gx_flash_light(GX_LIGHT_ID_5, &light);
}

static void scene_light_6(float time) {
    gx_light_t light;
    light.color = 0xFFFF00FF;

    vec3 target = vec3_new(sin(time * 4.34839) * 0.5 - 0.25, cos(time * 1.42328) * 0.5 -0.75f, -2.0);

    light.position = vec3_new(1.0f, 1.0f, 1.0);

    target = vec3_to(light.position, target);
    target = vec3_muls(target, -1.0); // hardware direction negative
    light.direction = target;


    float cutoff = 10.0 * M_PI / 180.0f;
    float cr = cosf(cutoff);
    light.cos_attenuation = vec3_new(-cr/(1.0f-cr), 0.0f, 1.0f/(1.0f-cr));
    light.distance_attenuation = vec3_new(1.0f, 0.0, 0.0);
    gx_flash_light(GX_LIGHT_ID_6, &light);
}

static void setup_cool_tev_lighting() {
    gx_flash_xf_color(GX_XF_COLOR_AMBIENT_0, 64, 64, 64, 255);
    gx_flash_xf_color(GX_XF_COLOR_MATERIAL_0, 255, 255, 255, 255);

    gx_configure_color_channel(GX_COLOR_CHANNEL_COLOR0, GX_LIGHT_BIT_0, true, false, false, GX_DIFFUSE_CLAMPED, GX_ATTENUATION_MODE_SPOTLIGHT);

    gx_flash_tev_register_color(GX_TEV_IO_REGISTER_0, 25, 35, 204, 255); // Hot Color
    gx_flash_tev_register_color(GX_TEV_IO_REGISTER_1, 255, 255, 255, 255); // Cold Color

    gx_tev_stage_t texturizer;
    gx_initialize_tev_stage(&texturizer);
    gx_set_tev_stage_color_input(&texturizer, GX_TEV_IO_REGISTER_0, GX_TEV_IO_REGISTER_1, GX_TEV_IO_TEXTURE, GX_TEV_IO_ZERO);

    gx_tev_stage_t lighting;
    gx_initialize_tev_stage(&lighting);
    gx_set_tev_stage_color_input(&lighting, GX_TEV_IO_ZERO, GX_TEV_IO_PREVIOUS, GX_TEV_IO_RASTERIZER, GX_TEV_IO_ZERO);

    gx_set_tev_stages(2);
    gx_flash_tev_stage(GX_TEV_STAGE_0, &texturizer);
    gx_flash_tev_stage(GX_TEV_STAGE_1, &lighting);
}

static void setup_texturing() {
    // Generate texture data
    gx_texture_t cool_texture;
    gx_initialize_texture(&cool_texture, utah_teapot_texture_data, GX_TEXTURE_FORMAT_I8,
        utah_teapot_texture_width, utah_teapot_texture_height, GX_WRAP_REPEAT, GX_WRAP_REPEAT, false);
    
    gx_flash_texture(GX_TEXTURE_MAP_0, &cool_texture);

    gx_configure_texcoord_channel(GX_TEXTURE_MAP_0, GX_TEXGEN_SOURCE_TEX0, GX_TEXGEN_TYPE_REGULAR, false, false, 0);

    gx_set_texcoord_channels(1);

    matrix34 scale_mtx;
    vec3 scale = vec3_new(1.0 / utah_teapot_model_texcoords_scale, -1.0 / utah_teapot_model_texcoords_scale, 1.0 / utah_teapot_model_texcoords_scale);
    matrix34_scale(scale_mtx, &scale);
    gx_flash_matrix(scale_mtx, GX_MTX_ID_1, false);
    gx_set_current_texcoord_matrix(GX_TEXTURE_MAP_0, GX_MTX_ID_1);
}

static void setup_vertex_formats() {
    // Vertex descriptors
    gx_vtxdesc_set(GX_VTXDESC_POSITION, GX_VTXATTR_DATA_DIRECT);
    gx_vtxdesc_set(GX_VTXDESC_NORMAL, GX_VTXATTR_DATA_DIRECT);
    gx_vtxdesc_set(GX_VTXDESC_TEXCOORD0, GX_VTXATTR_DATA_DIRECT);

    // Vertex Attribute 0
    gx_vtxfmtattr_set(0, GX_VTXDESC_POSITION, GX_VTXATTR_POS_XYZ, GX_VTXATTR_S16, 15);
    gx_vtxfmtattr_set(0, GX_VTXDESC_NORMAL, GX_VTXATTR_NRM_XYZ, GX_VTXATTR_S16, 1);
    gx_vtxfmtattr_set(0, GX_VTXDESC_TEXCOORD0, GX_VTXATTR_TEX_ST, GX_VTXATTR_S16, 0);

    // Generate 1 color channel
    gx_set_color_channels(1);

    // Lighting
    //gx_flash_xf_color(GX_XF_COLOR_AMBIENT_0, 12, 12, 12, 255);
    //gx_flash_xf_color(GX_XF_COLOR_MATERIAL_0, 230, 229, 225, 255);

    //gx_configure_color_channel(GX_COLOR_CHANNEL_COLOR0, GX_LIGHT_BIT_0 | GX_LIGHT_BIT_1 | GX_LIGHT_BIT_2 | GX_LIGHT_BIT_3 | GX_LIGHT_BIT_4 | GX_LIGHT_BIT_5 | GX_LIGHT_BIT_6, true, false, false, GX_DIFFUSE_CLAMPED, GX_ATTENUATION_MODE_SPOTLIGHT);

    setup_cool_tev_lighting();
    setup_texturing();
}

void setup_teapot_matrix(float time)
{
    matrix34 rotation;
    matrix34 translation;
    matrix34 translation_camera;
    matrix34 scale;
    matrix34 model;

    // Build rotation about Y
    matrix34_rotate_y(rotation, time);

    // Translation
    vec3 ts = { 0.0f, -0.75f, -2.0f };
    matrix34_translation(translation, &ts);

    // Model = Translation * Rotation
    matrix34_multiply(model, translation, rotation);

    // --- Position matrix ---
    gx_flash_matrix(model, GX_MTX_ID_0, true);

    // --- Normal matrix ---
    // Extract 3x3 from model, invert-transpose for normals
    matrix3 nrm;
    matrix3_from34(nrm, model);
    matrix3_inverse(nrm, nrm);
    matrix3_transpose(nrm, nrm);

    gx_flash_nrm_matrix(nrm, GX_MTX_ID_0);

    // Set position and normal matrix pair
    gx_set_current_psn_matrix(GX_MTX_ID_0);
}

int main() {
    // First init IOS pso we can use video modes.
    ios_initialize();

    // Get default video mode from IOS and use it to initialize the video interface.
    video_mode_t tv_mode = video_system_default_video_mode();
    video_initialize(tv_mode);
    video_set_retrace_callback(copy_framebuffer);
    video_set_framebuffer(&frame_buffer);

    // Initialize the graphics API
    const video_profile_t* profile = video_get_profile(tv_mode);
    gx_fifo_t fifo;
    gx_fifo_initialize(&fifo, fifo_buffer, sizeof(fifo_buffer));
    gx_initialize(&fifo, profile);

    setup_vertex_formats();
    setup_texturing();

    setup_scene();

    // Make sure all our setup is flushed before we beginin working
    gx_flush();

    game_loop();

    return 0;
}

static void game_loop() {
    scene_light_0();

    float time = 0.0f;
    while(true) {
        // Draw teapot
        scene_light_1(time);
        scene_light_2(time);
        scene_light_3(time);
        scene_light_4(time);
        scene_light_5(time);
        scene_light_6(time);
        setup_teapot_matrix(time);

        time += 1.0f / 60.0f;

        utah_teapot_draw();

        gx_draw_done();

        // Allow copy, wait till next frame
        framebuffer_ready = true;
        video_wait_vsync();
    }
}

static void copy_framebuffer() {
    // Copy frame buffers during the video retrace period
    if(framebuffer_ready) {
        gx_copy_framebuffer(&frame_buffer, true);
        gx_flush(); // Important you flush it!

        framebuffer_ready = false;
    }
}