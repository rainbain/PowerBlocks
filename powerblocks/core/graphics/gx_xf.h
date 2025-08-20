/**
 * @file gx_xf.h
 * @brief XF definitions and function of GX.
 *
 * XF definitions and function of GX.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

// This header is not supposed to be included directly.
// Instead include gx.h

#include <stdint.h>

#include "powerblocks/core/utils/math/matrix3.h"
#include "powerblocks/core/utils/math/matrix4.h"
#include "powerblocks/core/utils/math/matrix34.h"

/* -------------------XF Matrix Control--------------------- */

typedef enum { // Row of the matrix. As compared to their addressing since normal matrixes are smaller
    GX_PSNMTX_0 = 0,
    GX_PSNMTX_1 = 3,
    GX_PSNMTX_2 = 6,
    GX_PSNMTX_3 = 9,
    GX_PSNMTX_4 = 12,
    GX_PSNMTX_5 = 15,
    GX_PSNMTX_6 = 18,
    GX_PSNMTX_7 = 21,
    GX_PSNMTX_8 = 24,
    GX_PSNMTX_9 = 27,
} gx_psnmtx_idx;

/**
 * @brief Flashes the viewport onto the XF settings.
 *
 * Set the viewport in the XF registers.
 * This is needed to scale geometry to the screen.
 * It also handles scaling Z buffer.
 * 
 * @param x Top Left of the Screen Position
 * @param y Top Left of the Screen Position
 * @param width Width of the Screen
 * @param height Height of the Screen
 * @param near Closest Z values to not be culled
 * @param far Farthest Z values to not be culled
 * @param jitter Will shift up the X a bit. Can reduce blur/flickering in interlaced mode. This is usually true.
 */
extern void gx_flash_viewport(float x, float y, float width, float height, float near, float far, bool jitter);

/**
 * @brief Flashes the projection matrix onto the XF settings
 * 
 * Sets the projection matrix in the XF registers
 * that is used when transforming vertices.
 * 
 * @param mtx 4x4 Projection Matrix
 * @param is_perspective If the matrix is perspective, or orthographic. 
*/
extern void gx_flash_projection(const matrix4 mtx, bool is_perspective);

/**
 * @brief Fashes a position matrix in the XF matrix stack
 * 
 * You then can set this with gx_set_current_psn_matrix or with matrix indices in vertex attributes.
 * 
 * @param mtx The matrix to load
 * @param index The index of the matrix in the matrix stack.
 */
extern void gx_flash_pos_matrix(const matrix34 mtx, gx_psnmtx_idx index);

/**
 * @brief Fashes a normal matrix into the XF matrix stack
 * 
 * You then can set this with gx_set_current_psn_matrix or with matrix indices in vertex attributes.
 * 
 * 
 * @param mtx The matrix to load
 * @param index The index of the matrix in the matrix stack.
 */
extern void gx_flash_nrm_matrix(const matrix3 mtx, gx_psnmtx_idx index);

/**
 * @brief Sets the current pair of position and normal matrices.
 * 
 * 10 matrices can be set in the XF, set with functions like gx_flash_projection.
 * 
 * This will set the current one to use, if matrix indices are not enabled in the vertex descriptor.
 * If they are, this will be overriden.
 * 
 * This value will be flushed on the next draw call.
 * 
 * Cleared to zero by gx_initialize()
 */
extern void gx_set_current_psn_matrix(gx_psnmtx_idx index);

/* -------------------XF Color Control--------------------- */

typedef enum { // These are just the XF register addresses for them
    GX_COLOR_CHANNEL_COLOR0 = 0,
    GX_COLOR_CHANNEL_COLOR1 = 1,
    GX_COLOR_CHANNEL_ALPHA0 = 2,
    GX_COLOR_CHANNEL_ALPHA1 = 3
} gx_color_channel_t;

typedef enum {
    GX_LIGHT_BIT_NONE,
    GX_LIGHT_BIT_0 = (1<<2),
    GX_LIGHT_BIT_1 = (1<<3),
    GX_LIGHT_BIT_2 = (1<<4),
    GX_LIGHT_BIT_3 = (1<<5),
    GX_LIGHT_BIT_4 = (1<<11),
    GX_LIGHT_BIT_5 = (1<<12),
    GX_LIGHT_BIT_6 = (1<<13),
    GX_LIGHT_BIT_7 = (1<<14)
} gx_light_bit_t;

typedef enum {
    // Bit 1 is just the attenuation enable
    // Bit 0 is the select. We just turned the binary to number here.
    GX_ATTENUATION_MODE_NONE,
    GX_ATTENUATION_MODE_SPECULAR = 1,
    GX_ATTENUATION_MODE_SPOTLIGHT = 3
} gx_attenuation_mode_t;

typedef enum {
    GX_DIFFUSE_MODE_NONE,
    GX_DIFFUSE_SIGNED,
    GX_DIFFUSE_CLAMPED, // From [0, 1]
} gx_diffuse_mode_t;

typedef enum {
    GX_XF_COLOR_AMBIENT_0 = 0x100A,
    GX_XF_COLOR_AMBIENT_1 = 0x100B,
    GX_XF_COLOR_MATERIAL_0 = 0x100C,
    GX_XF_COLOR_MATERIAL_1 = 0x100D
} gx_xf_color_t;

typedef enum { // Just the addresses in XF memory.
    GX_LIGHT_ID_0 = 0x0600,
    GX_LIGHT_ID_1 = 0x0610,
    GX_LIGHT_ID_2 = 0x0620,
    GX_LIGHT_ID_3 = 0x0630,
    GX_LIGHT_ID_4 = 0x0640,
    GX_LIGHT_ID_5 = 0x0650,
    GX_LIGHT_ID_6 = 0x0660,
    GX_LIGHT_ID_7 = 0x0670
} gx_light_id_t;

typedef struct {
    uint32_t color; // RGBA

    // Light attenuation!
    // Described as a quadratic with
    // Angle is the light direction, and the vertex to light. basically the dot product/cosine
    // attn.x + attn.y * cos(angle) + attn.z * cos(angle)^2
    vec3 cos_attenuation;
    
    // Same quadratic idea, inverse this time
    vec3 distance_attenuation;

    vec3 position;

    // These are negative by the way
    // Times them by -1. Fun hardware
    vec3 direction;
} gx_light_t;

/**
 * @brief Sets a color channel count
 *
 * The vertex descriptors contain how many colors are passed to the XF.
 * This controls how many of those colors are passed to the BP/TEV from the XF.
 * 
 * These colors will be rasterized and interpolated.
 * 
 * Hardware supports 0-2 colors.
 * 
 * @param count Number of color channels to supply the BP/TEV
 */
extern void gx_set_color_channels(uint32_t count);

/**
 * @brief Configures 1 of the 2/4 color channels.
 * 
 * There are 2 color channels, like in `gx_set_color_channels`
 * Now each has settings for how its transformed for lighting.
 * 
 * Unlike `gx_set_color_channels` though this has its own 4 channels.
 * 2 colors, 2 alphas.
 * 
 * Default config just passes the vertex colors with no modifications.
 * 
 * @param channel Color channel to use
 * @param lights Enable lights, like GX_LIGHT_ENABLE_0 | GX_LIGHT_ENABLE_1
 * @param lighting Enable lighting in general, so you can have a dark scene with no lights if you wish
 * @param ambient_source True = use vertex color, otherwise use XF register
 * @param material_source True = use vertex color, otherwise use XF register.
 * @param diffuse Diffuse Mode to use
 * @param attenuation Attenuation Mode to use
*/
extern void gx_configure_color_channel(gx_color_channel_t channel, gx_light_bit_t lights,
                                       bool lighting, bool ambient_source, bool material_source,
                                       gx_diffuse_mode_t diffuse, gx_attenuation_mode_t attenuation);

/**
 * @brief Sets a XF Color Value
 */
extern void gx_flash_xf_color(gx_xf_color_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * @brief Flash one of the lights.
 * 
 * Copys the light data structure to XF memory of one of the 1-8 lights
 * 
 * @param id ID of the light to use
 * @param light Light data structure
 */
extern void gx_flash_light(gx_light_id_t id, const gx_light_t* light);