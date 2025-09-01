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

/**
 * @enum gx_mtx_id_t
 * @brief Universal matrix ids.
 *
 * These are used for addressing geometry, normal, texcoord matrices
 * and dual texcoord matrices.
 * 
 * Depending on what of these its for, some or all of these may be valid.
 * 
 * Geometry and texcoord share the same memory, and use GX_MTX_ID_0
 * through GX_MTX_ID_20. Where GX_MTX_ID_20 by default
 * is loaded as an idiniticy matrix for them to use.
 * 
 * Normal matrices can only use GX_MTX_ID_0 through GX_MTX_ID_9.
 * 
 * Dual texcoords have their own stack and can use GX_MTX_ID_0, GX_MTX_ID_20.
 */
typedef enum {
    GX_MTX_ID_0  = (0*3),
    GX_MTX_ID_1  = (1*3),
    GX_MTX_ID_2  = (2*3),
    GX_MTX_ID_3  = (3*3),
    GX_MTX_ID_4  = (4*3),
    GX_MTX_ID_5  = (5*3),
    GX_MTX_ID_6  = (6*3),
    GX_MTX_ID_7  = (7*3),
    GX_MTX_ID_8  = (8*3),
    GX_MTX_ID_9  = (9*3),
    GX_MTX_ID_10 = (10*3),
    GX_MTX_ID_11 = (11*3),
    GX_MTX_ID_12 = (12*3),
    GX_MTX_ID_13 = (13*3),
    GX_MTX_ID_14 = (14*3),
    GX_MTX_ID_15 = (15*3),
    GX_MTX_ID_16 = (16*3),
    GX_MTX_ID_17 = (17*3),
    GX_MTX_ID_18 = (18*3),
    GX_MTX_ID_19 = (19*3),
    GX_MTX_ID_20 = (20*3),

    GX_MTX_ID_IDENTITY = GX_MTX_ID_20
} gx_mtx_id_t;

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
 * @brief Fashes a matrix to the XF Stack
 * 
 * The same matrix memory is shared for geometry matrices and texcoord
 * transformation matrices. GX_MTX_ID_IDENTITY is automatically flashed
 * with an identity matrices during initialization and is the default
 * matrix in texcoord and geometry generations.
 * 
 * The IDs themselves address 3x4 matrices, but hardware does support using
 * 2x4 matrices. In this case, you can fit more matrices. Since having both
 * 2x4 and 3x4 matrices in the same memory space without an allocator
 * gets a bit messy, I will leave it up to you to use that if needed.
 * 
 * @param mtx The matrix to load
 * @param index The index of the matrix in the matrix stack.
 * @param type If type is true, a full 3x4 matrices will be loaded, otherwise its just 2x4.
 */
extern void gx_flash_matrix(const matrix34 mtx, gx_mtx_id_t index, bool type);

/**
 * @brief Fashes a normal matrix into the XF matrix stack
 * 
 * Normal matrices have their own separate stack.
 * 
 * Only the first 10 matrices, GX_MTX_ID_0 through GX_MTX_ID_9,
 * exist in the normal matrix stack.
 * 
 * You then can set this with gx_set_current_psn_matrix or with matrix indices in vertex attributes.
 * 
 * @param mtx The matrix to load
 * @param index The index of the matrix in the matrix stack.
 */
extern void gx_flash_nrm_matrix(const matrix3 mtx, gx_mtx_id_t index);

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
 * Cleared to GX_MTX_ID_IDENTITY by gx_initialize()
 */
extern void gx_set_current_psn_matrix(gx_mtx_id_t index);

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

typedef enum { // Instead of raw XF addresses, indexes, so it can be used in texcoord gens.
    GX_LIGHT_ID_0,
    GX_LIGHT_ID_1,
    GX_LIGHT_ID_2,
    GX_LIGHT_ID_3,
    GX_LIGHT_ID_4,
    GX_LIGHT_ID_5,
    GX_LIGHT_ID_6,
    GX_LIGHT_ID_7
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

/* -------------------XF TexCoord Control--------------------- */

typedef enum {
    GX_TEXGEN_TYPE_REGULAR,
    GX_TEXGEN_TYPE_EMBOSS,
    GX_TEXGEN_TYPE_COLOR_0,
    GX_TEXGEN_TYPE_COLOR_1
} gx_texgen_type_t;

typedef enum {
    GX_TEXGEN_SOURCE_POSITION,
    GX_TEXGEN_SOURCE_NORMAL,
    GX_TEXGEN_SOURCE_COLORS,
    GX_TEXGEN_SOURCE_BINORMAL_T,
    GX_TEXGEN_SOURCE_BINORMAL_B,
    GX_TEXGEN_SOURCE_TEX0,
    GX_TEXGEN_SOURCE_TEX1,
    GX_TEXGEN_SOURCE_TEX2,
    GX_TEXGEN_SOURCE_TEX3,
    GX_TEXGEN_SOURCE_TEX4,
    GX_TEXGEN_SOURCE_TEX5,
    GX_TEXGEN_SOURCE_TEX6,
    GX_TEXGEN_SOURCE_TEX7
} gx_texgen_source_t;

typedef enum {
    GX_TEXTURE_MAP_0,
    GX_TEXTURE_MAP_1,
    GX_TEXTURE_MAP_2,
    GX_TEXTURE_MAP_3,
    GX_TEXTURE_MAP_4,
    GX_TEXTURE_MAP_5,
    GX_TEXTURE_MAP_6,
    GX_TEXTURE_MAP_7
} gx_texture_map_t;

/**
 * @brief Set a texture coord count
 * 
 * Like gx_set_color_channels, set the number of texcoords that should be generated.
 * 
 * @param count Number of color channels to supply the BP/TEV
*/
extern void gx_set_texcoord_channels(uint32_t count);

/**
 * @brief Configures 1 of the 8 texcoord channels.
 * 
 * Sets up how a texture coordinate will be transformed from the XF.
 * 
 * The source contains what XF input is used for this texture coordinate.
 * This data is processed according to type, where you can change how colors are processed for example.
 * 
 * You can only use sources GX_TEXGEN_SOURCE_TEX0 through GX_TEXGEN_SOURCE_TEX7 for embossing,
 * since it calculates the embossing source to be the texgen source, but it can only use texcoords 0-7.
 * 
 * @param map Texture map to configure
 * @param gx_texgen_source_t Sets where the data will be pulled from.
 * @param gx_texgen_type Sets how inputs are processed. As eather normal values, emboss, or from color data
 * @param projection If set 3x4 texture matrices are used instead. For 3D texcoords.
 * @param three_component Is set, (X, Y, Z, 1.0) is passed, otherwise its just (X, Y, 1.0, 1.0)
 * @param embossing_light Light used for embossing.
*/
extern void gx_configure_texcoord_channel(gx_texture_map_t map, gx_texgen_source_t source, gx_texgen_type_t type, bool projection, bool three_component, gx_light_id_t embossing_light);

/**
 * @brief Configure dual texcoords.
 * 
 *  * You can only use these on GX_TEXTURE_MAP_0 through GX_TEXTURE_MAP_3
 * 
 * These allow to transform texcoords after the main transformation.
 * This means the main transformation can be used for. i.e. putting geometry into a texture format,
 * then you can use this to scroll around that texture.
 * 
 * Its often that if your doing this, you may want to normalize the inputs as well before the first transformation.
 * 
 * @param map Texture map to configure
 * @param index Matrix index to transform texcoord after transformation
 * @param normalize Normalize the inputs before transforming 
*/
extern void gx_configure_dual_texcoord(gx_texture_map_t map, gx_mtx_id_t index, bool normalize);

/**
 * @brief Fashes a dual texcoord matrix in the XF matrix stack
 * 
 * GX_MTX_ID_0 through GX_MTX_ID_20 is valid for this.
 * 
 * @param mtx Matrix to flash
 * @param index Matrix index to flash into.
 * @param type If type is true, the full 3x4 will be loaded, otherwise 2x4 will be loaded
 */
extern void gx_flash_dual_texcoord_matrix(matrix34 mtx, gx_mtx_id_t index, bool type);

/**
 * @brief Sets the current texcoord matrix.
 * 
 * This is the current matrix used to transform the texcoord generation
 * input.
 * 
 * Set to GX_MTX_ID_IDENTITY by default.
 * 
 * Flushed in the next draw call
 * 
 * @param map Texture map to set
 * @param index Matrix index to set it to
 */
void gx_set_current_texcoord_matrix(gx_texture_map_t map, gx_mtx_id_t index);

/**
 * @brief Sets the Dual Texture Coord Enable/Disable Register
 * 
 * If disabled, as is by default,
 * non will be transformed.
 * 
 * If enabled, all dual tex coords will be transformed.
 */
void gx_flash_enable_dual_texcoord(bool enable);