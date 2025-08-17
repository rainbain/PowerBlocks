/**
 * @file gx.h
 * @brief Manages graphics on the system.
 *
 *  Manages graphics on the system. Provides a interface to create and send
 *  commands to hardware, and configure the hardware for rendering.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "powerblocks/core/utils/math/matrix4.h"
#include "powerblocks/core/utils/math/matrix34.h"

#include "framebuffer.h"
#include "video.h"

#define GX_WPAR_ADDRESS 0xCC008000

// The write gather pipeline will collect writes to these and
// put them into the GX FIFO
#define GX_WPAR_U8  (*(volatile uint8_t*)GX_WPAR_ADDRESS)
#define GX_WPAR_U16 (*(volatile uint16_t*)GX_WPAR_ADDRESS)
#define GX_WPAR_U32 (*(volatile uint32_t*)GX_WPAR_ADDRESS)
#define GX_WPAR_S8  (*(volatile int8_t*)GX_WPAR_ADDRESS)
#define GX_WPAR_S16 (*(volatile int16_t*)GX_WPAR_ADDRESS)
#define GX_WPAR_S32 (*(volatile int32_t*)GX_WPAR_ADDRESS)
#define GX_WPAR_F32 (*(volatile float*)GX_WPAR_ADDRESS)

#include "gx_immediate.h"

// Vertex Data Type
typedef enum {
    GX_VTXATTR_DATA_DISABLED,
    GX_VTXATTR_DATA_DIRECT,
    GX_VTXATTR_DATA_INDEX8,
    GX_VTXATTR_DATA_INDEX16
} gx_vtxattr_data_t;

typedef enum {
    GX_VTXDESC_POSNORM_INDEX,
    GX_VTXDESC_TEXCOORDMTX0,
    GX_VTXDESC_TEXCOORDMTX1,
    GX_VTXDESC_TEXCOORDMTX2,
    GX_VTXDESC_TEXCOORDMTX3,
    GX_VTXDESC_TEXCOORDMTX4,
    GX_VTXDESC_TEXCOORDMTX5,
    GX_VTXDESC_TEXCOORDMTX6,
    GX_VTXDESC_TEXCOORDMTX7,
    GX_VTXDESC_POSITION,
    GX_VTXDESC_NORMAL,
    GX_VTXDESC_NORMAL_NBT,
    GX_VTXDESC_COLOR0,
    GX_VTXDESC_COLOR1,
    GX_VTXDESC_TEXCOORD0,
    GX_VTXDESC_TEXCOORD1,
    GX_VTXDESC_TEXCOORD2,
    GX_VTXDESC_TEXCOORD3,
    GX_VTXDESC_TEXCOORD4,
    GX_VTXDESC_TEXCOORD5,
    GX_VTXDESC_TEXCOORD6,
    GX_VTXDESC_TEXCOORD7
} gx_vtxdesc_t;

typedef enum {
    GX_VTXATTR_POS_XY = 0,
    GX_VTXATTR_POS_XYZ = 1,

    GX_VTXATTR_RGB = 0,
    GX_VTXATTR_RGBA = 1,

    GX_VTXATTR_NRM_XYZ = 0,
    GX_VTXATTR_NRM_NBT = 1,
    GX_VTXATTR_NRM_NBT3 = 3,

    GX_VTXATTR_TEX_S = 0,
    GX_VTXATTR_TEX_ST = 1
} gx_vtxattr_component_t;

typedef enum {
    GX_VTXATTR_U8 = 0,
    GX_VTXATTR_S8,
    GX_VTXATTR_U16,
    GX_VTXATTR_S16,
    GX_VTXATTR_F32,

    GX_VTXATTR_RGB565 = 0,
    GX_VTXATTR_RGB8,
    GX_VTXATTR_RGBX8,
    GX_VTXATTR_RGBA4,
    GX_VTXATTR_RGBA6,
    GX_VTXATTR_RGBA8
} gx_vtxattr_component_format_t;

typedef enum {
    GX_LINES = 0xA8,
    GX_LINESTRIP = 0xB0,
    GX_POINTS = 0xB8,
    GX_QUADS = 0x80,
    GX_TRIANGLE_FAN = 0xA0,
    GX_TRIANGLES = 0x90,
    GX_TRIANGLE_STRIP = 0x98
} gx_primitive_t;

typedef enum { // Just their addresses/offset in XF memory for position and normal matrices.
    GX_PSNMTX_0 = (12*0),
    GX_PSNMTX_1 = (12*1),
    GX_PSNMTX_2 = (12*2),
    GX_PSNMTX_3 = (12*3),
    GX_PSNMTX_4 = (12*4),
    GX_PSNMTX_5 = (12*5),
    GX_PSNMTX_6 = (12*6),
    GX_PSNMTX_7 = (12*7),
    GX_PSNMTX_8 = (12*8),
    GX_PSNMTX_9 = (12*9)
} gx_psnmtx_idx;

typedef enum {
    GX_COMPARE_NEVER,
    GX_COMPARE_LESS,
    GX_COMPARE_EQUAL,
    GX_COMPARE_LESS_EQUAL,
    GX_COMPARE_GREATER,
    GX_COMPARE_NOT_EQUAL,
    GX_COMPARE_GREATER_EQUAL,
    GX_COMPARE_ALWAYS
} gx_compare_t;

typedef enum {
    GX_PIXEL_FORMAT_RGB8_Z24,
    GX_PIXEL_FORMAT_RGBA6_Z24,
    GX_PIXEL_FORMAT_RGB565_Z16,
    GX_PIXEL_FORMAT_Z24,
    GX_PIXEL_FORMAT_Y8,
    GX_PIXEL_FORMAT_U8,
    GX_PIXEL_FORMAT_V8,
    GX_PIXEL_FORMAT_YUV420
} gx_pixel_format_t;

typedef enum {
    GX_Z_FORMAT_LINEAR,
    GX_Z_FORMAT_NEAR,
    GX_Z_FORMAT_MID,
    GX_Z_FORMAT_FAR
} gx_z_format_t;

typedef enum {
    GX_CLAMP_MODE_NONE,
    GX_CLAMP_MODE_TOP,
    GX_CLAMP_MODE_BOTTOM,
    GX_CLAMP_MODE_TOP_BOTTOM,
} gx_clamp_mode_t;

typedef enum {
    GX_GAMMA_1_0, // 1.0
    GX_GAMMA_1_7, // 1.7
    GX_GAMMA_2_2 // 2.2
} gx_gamma_t;

typedef enum {
    GX_LINE_MODE_PROGRESSIVE,
    GX_LINE_MODE_EVEN = 2,
    GX_LINE_MODE_ODD,
} gx_line_mode_t;

#define GX_WPAR_OPCODE_LOAD_CP                 0x08
#define GX_WPAR_OPCODE_LOAD_XF                 0x10
#define GX_WPAR_OPCODE_LOAD_BP                 0x61
#define GX_WPAR_OPCODE_PRIMITIVE(type, format) (((uint8_t)type) | format)

#define GX_XF_REGISTER_VERTEX_STATS   0x1008
#define GX_XF_REGISTER_COLOR_COUNT    0x1009
#define GX_XF_REGISTERS_MTXIDX_A      0x1018
#define GX_XF_REGISTERS_MTXIDX_B      0x1019
#define GX_XF_REGISTER_VIEWPORT       0x101A
#define GX_XF_REGISTER_PROJECTION     0x1020
#define GX_XF_REGISTER_TEXCOORD_COUNT 0x103F
#define GX_XF_REGISTER_COLOR_CONTROL  0x100E

#define GX_BP_REGISTERS_GENMODE                      (0x00 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_POS_A            (0x01 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_POS_B            (0x02 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_POS_C            (0x03 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_POS_D            (0x04 << 24)
#define GX_BP_REGISTERS_SCISSOR_TL                   (0x20 << 24)
#define GX_BP_REGISTERS_SCISSOR_BR                   (0x21 << 24)
#define GX_BP_REGISTERS_Z_MODE                       (0x40 << 24)
#define GX_BP_REGISTERS_CMODE_0                      (0x41 << 24)
#define GX_BP_REGISTERS_CMODE_1                      (0x42 << 24)
#define GX_BP_REGISTERS_PE_CONTROL                   (0X43 << 24)
#define GX_BP_REGISTERS_PE_DONE                      (0x45 << 24)
#define GX_BP_REGISTERS_EFB_SOURCE_TOP_LEFT          (0x49 << 24)
#define GX_BP_REGISTERS_EFB_SOURCE_WIDTH_HEIGHT      (0x4A << 24)
#define GX_BP_REGISTERS_XFB_TARGET_ADDRESS           (0x4B << 24)
#define GX_BP_REGISTERS_EFB_DESTINATION_WIDTH        (0x4D << 24)
#define GX_BP_REGISTERS_Y_SCALE                      (0x4E << 24)
#define GX_BP_REGISTERS_PE_COPY_CLEAR_AR             (0x4F << 24)
#define GX_BP_REGISTERS_PE_COPY_CLEAR_GB             (0x50 << 24)
#define GX_BP_REGISTERS_PE_COPY_CLEAR_Z              (0x51 << 24)
#define GX_BP_REGISTERS_PE_COPY_EXECUTE              (0x52 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_COEFF_A          (0x53 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_COEFF_B          (0x54 << 24)
#define GX_BP_REGISTERS_SCISSOR_OFFSET               (0x59 << 24)

#define GX_CP_REGISTERS_MTXIDX_A 0x30
#define GX_CP_REGISTERS_MTXIDX_B 0x40
#define GX_CP_REGISTER_VCD_LOW   0x50
#define GX_CP_REGISTER_VCD_HIGH  0x60
#define GX_CP_REGISTER_VAT_A     0x70
#define GX_CP_REGISTER_VAT_B     0x80
#define GX_CP_REGISTER_VAT_C     0x90

// Load a 32 bit value into the command processor
// through the pipeline.
#define GX_WPAR_CP_LOAD(address, value) \
    GX_WPAR_U8 = GX_WPAR_OPCODE_LOAD_CP;          /* Load CP Command */ \
    GX_WPAR_U8 = address;                         /* Register Address */ \
    GX_WPAR_U32 = value;                          /* Register Value */

// Load some amount of 32 bit values onto the XF
// To be followed with each value on the WPAR.
#define GX_WPAR_XF_LOAD(address, words) \
    GX_WPAR_U8 = GX_WPAR_OPCODE_LOAD_XF;          /* Load XF Command */ \
    GX_WPAR_U16 = (words) - 1;                     /* Load N-1 32 bit values */ \
    GX_WPAR_U16 = address;                        /* Register Address */

// Loads a BP register. The upper 8 bits are the register address
// The lower 24 are the value. We have just combined them here due to avoid dealing with 24 bit imm
#define GX_WPAR_BP_LOAD(value) \
    GX_WPAR_U8 = GX_WPAR_OPCODE_LOAD_BP; /* Load BP Command */ \
    GX_WPAR_U32 = value; /* Combined 8 bit address and 24 bit value. */


/**
 * @brief Represents the a fifo for the Command Processor
 *
 * Exactly follows the registers for the command processor's FIFO.
 * 
 * Such this data structure is used to set a fifo, or get info about
 * the current fifo.
 */
typedef struct {
    uint32_t base_address;
    uint32_t end_address;
    uint32_t high_watermark;
    uint32_t low_watermark;
    uint32_t distance;
    uint32_t write_head;
    uint32_t read_head;
    uint32_t breakpoint;
} gx_fifo_t;

/**
 * @brief Initializes the graphics.
 *
 * Puts the system graphics into a default state ready
 * for rendering.
 * 
 * Takes a 32 byte aligned FIFO to hold commands. Hardware automatically
 * fills it and cycles back through it, but write commands too fast and its
 * possible to overflow it.
 * @param fifo Fifo to use.
 * @param video_profile Video profile to use.
 */
extern void gx_initialize(const gx_fifo_t* fifo, const video_profile_t* video_profile);

/**
 * @brief Initializes the graphics state.
 *
 * This function, called by gx_initialize, configures the hardware into GX's default state.
 * That includes all the TEV stages and stuff like that.
 * 
 * It does not force the pipeline into a known state if somethings gone horribly wrong.
 * Its just a bunch of calls to make the hardware into a pretty state.
 * 
 * This does not initialize some of the video settings. Use initialize video for that.
*/
extern void gx_initialize_state();

/**
 * @brief Initializes all the video settings from a video profile.
 * 
 * This function, usually following gx_initialize_state, can take a video profile
 * and configure all the EFB and XFB copy registers to match your video profile.
*/
extern void gx_initialize_video(const video_profile_t* video_profile);

/**
 * @brief Creates the fifo data structure.
 *
 * Creates a simple fifo data structure to set in the CP.
 * 
 * @param fifo The fifo to setup
 * @param fifo_buffer Buffer to put data into. 32 byte aligned
 * @param fifo_buffer_size Size of the buffer. 32 byte aligned
 */
extern void gx_fifo_create(gx_fifo_t* fifo, void* fifo_buffer, uint32_t fifo_buffer_size);

/**
 * @brief Sets the current FIFO
 *
 * Sets the current FIFO the CP will read from.
 * CP is hopefully disabled when you do this.
 * 
 * gx_initialize calls this
 * 
 * @param fifo The fifo to set
 */
extern void gx_fifo_set(const gx_fifo_t* fifo);

/**
 * @brief Gets the current FIFO
 *
 * Gets the current fifo from hardware.
 * Useful to check up on the CP.
 * 
 * @param fifo Writes to fifo data into this.
 */
extern void gx_fifo_get(gx_fifo_t* fifo);

/**
 * @brief Sets the Viewport
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
extern void gx_set_viewport(float x, float y, float width, float height, float near, float far, bool jitter);

/**
 * @brief Sets the Projection Matrix
 * 
 * Sets the projection matrix in the XF registers
 * that is used when transforming vertices.
 * 
 * @param mtx 4x4 Projection Matrix
 * @param is_perspective If the matrix is perspective, or orthographic. 
*/
extern void gx_set_projection(const matrix4 mtx, bool is_perspective);

/**
 * @brief Load a position matrix in the XF matrix stack
 * 
 * You then can set this with gx_set_psn_matrix or with matrix indices in vertex attributes.
 * 
 * @param mtx The matrix to load
 * @param index The index of the matrix in the matrix stack.
 */
extern void gx_load_psn_matrix(const matrix34 mtx, gx_psnmtx_idx index);

/**
 * @brief Sets the current pair of position and normal matrices.
 * 
 * 10 matrices can be set in the XF, set with functions like gx_set_projection.
 * 
 * This will set the current one to use, if matrix indices are not enabled in the vertex descriptor.
 * If they are, this will be overriden.
 * 
 * This value will be flushed on the next draw call.
 * 
 * Cleared to zero by gx_initialize()
 */
extern void gx_set_psn_matrix(gx_psnmtx_idx index);

/**
 * @brief Flushes all commands to the command processor.
 *
 * This inserts 32 NOP commands into the write gather pipeline to make sure
 * that any commands at the end of the FIFO or display list get sent, since
 * it only reads them in 32 byte burst.
 * 
 * This does not wait for command completion, it simply confirms commands before
 * it will not be lost.
 */
extern void gx_flush();

/**
 * @brief Grabs a pixel from the embedded frame buffer.
 *
 * Grabs a pixel from the embedded frame buffer.
 * This is useful for debugging.
 * 
 * I don't recommend using it too much, the emulators do not like this.
 * And it usually needs to be enabled for it to work in emulator.
 */
extern uint32_t gx_efb_peak(uint32_t x, uint32_t y);

/**
 * @brief Creates an empty vertex descriptor.
 *
 * Creates a simple fifo data structure to set in the CP.
 * 
 * @param fifo The fifo to setup
 * @param fifo_buffer Buffer to put data into. 32 byte aligned
 * @param fifo_buffer_size Size of the buffer. 32 byte aligned
 */
extern void gx_fifo_create(gx_fifo_t* fifo, void* fifo_buffer, uint32_t fifo_buffer_size);

/**
 * @brief Clears out all vertex descriptions.
 *
 * Turns off all vertex description parts in the VCD.
 * 
 * Enable them with gx_vtxdesc_set.
 * 
 * Guarantees it will be flushed to a valid state on the next draw call.
 * 
 * Called by gx_initialize()
 */
extern void gx_vtxdesc_clear();

/**
 * @brief Sets a vertex description.
 *
 * Sets the source of vertex data the attributes will use.
 * 
 * Note, only positions, normals, colors, and texture coords support
 * indexed types, the remaining will only be enabled or disabled.
 * 
 * There can only be 1 vertex description, but 8 vertex attributes.
 * 
 * Requires a vertex data cache flushed before use of new format.
 * 
 * @param desc Vertex description to enable. 
 * @param type Type of vertex data for it.
 */
extern void gx_vtxdesc_set(gx_vtxdesc_t desc, gx_vtxattr_data_t type);

/**
 * @brief Clears out a vertex attribute format.
 *
 * Clears out a vertex attribute format.
 * 
 * Set with gx_vtxfmtattr_set.
 * 
 * Called by gx_initialize()
 * 
 * Guarantees it will be flushed to a valid state on the next draw call.
 * 
 * @param attribute_index The vertex attribute table to use, 0-7.
 */
extern void gx_vtxfmtattr_clear(uint8_t attribute_index);

/**
 * @brief Sets the format of a vertex attribute
 * 
 * Sets the data format that vertex attributes are.
 * 
 * This will get updated in the VAT table of the GPU.
 * 
 * @param attribute The vertex attribute table to use, 0-7. Can switch between groups of vertex formats.
 * @param component The component, like is it XY, or XYZ. RGB or RGBA
 * @param fmt Format of the component, Is it RGBA8888, or like, U16 position data, or float
 * @param fraction Number of fractional bits in fixed point values. That is, inititer formats like S16, will be divided by 2^fraction. 0-31
 */
extern void gx_vtxfmtattr_set(uint8_t attribute_index, gx_vtxdesc_t attribute, gx_vtxattr_component_t component, gx_vtxattr_component_format_t fmt, uint8_t fraction);

/**
 * @brief Sets a color channel count
 *
 * The vertex descriptors contain how many colors are passed to the XF.
 * This controls how many of those colors are passed to the BP/TEV from the XF.
 * 
 * These colors will be rasterized and interpolated.
 * 
 * Hardware supports 0-2 colors.
 * Fun Fact: I hear the Nintendo SDK only supports 1, I cant imagine thats true though
 * Maybe just for model formats or something.
 * 
 * @param count Number of color channels to supply the BP/TEV
 */
extern void gx_set_color_channels(uint32_t count);

/**
 * @brief Set a texture coord count
 * 
 * Like gx_set_color_channels, set the number of texcoords that should be generated.
 * 
 * @param count Number of color channels to supply the BP/TEV
*/
extern void gx_set_texcoord_channels(uint32_t count);

/**
 * @brief Begin drawing primatives.
 * 
 * Signals to the GPU to expect primitive data in coming messages.
 * Also flushes GPU state changes.
 * 
 * @param primitive Type of 3D primitive to draw.
 * @param attribute Vertex attribute to use, values 0-7.
 * @param count Vertex count. Up to 65535
 */
extern void gx_begin(gx_primitive_t primitive, uint8_t attribute, uint16_t count);

/**
 * @brief End drawing primatives.
 * 
 * This does nothing, its what you see in things like OpenGL
 * so its included for similarity to such implementations.
 * Also means it can be expanded if it turns out to be needed
 * at some point.
 */
#define gx_end()

/**
 * @brief Call to send end current drawing session. Then wait.
 * 
 * This will signal to the BP that drawing is done,
 * flush it, then sleep the current thread until its done
*/
extern void gx_draw_done();

/**
 * @brief Sets the background color when copying framebuffers.
 * 
 * As the framebuffer is copied, it clears the EFB back with these colors
 * and Z value.
 * 
 * @param r Red Channel
 * @param g Green Channel
 * @param b Blue Channel
 * @param a Alpha Channel
 */
extern void gx_set_clear_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * @brief Sets the z value the frame buffer is cleared with.
 * 
 * As the frame buffer gets copied, it is cleared with this.
 * 
 * @param z Z value, usually the max of 0xFFFFFF (24 bits)
 */
extern void gx_set_clear_z(uint32_t z);

/**
 * @brief Sets the scale of the EFB to the XFB's Y
 * 
 * Scales the image on the y axis to the size of the XFB
 * 
 * @param scale Usually xfb height / efb height
 */
extern void gx_set_copy_y_scale(float y_scale);

/**
 * @brief Sets the scissor rectangle.
 * 
 * Defines the rectangle things will be culled in.
 * 
 * Should be set to the viewport size by default.
 * 
 * Values range from 0 to 2047
 * gx_set_scissor_offset can be used to further expand this.
 * 
 * @param x Begining / Left of the Window
 * @param y Begining / Top of the Window
 * @param width Width of thw window
 * @param height Height of the Window
 */
extern void gx_set_scissor_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

/**
 * @brief Offsets the scissor rectangle.
 * 
 * Allows the scissor rectangle to be moved around the screen.
 * 
 * Values range from -342 to 382 and are even.
 * 
 * @param x X offset of the window
 * @param y Y offset of the window.
 */
extern void gx_set_scissor_offset(int32_t x, int32_t y);

/**
 * @brief Sets the window to of the EFB into the XFB
 * 
 * Sets the window, or region, of the EFB to copy from.
 * 
 * @param x Begining / Left of the EFB Window
 * @param y Begining / Top of the EFB Window
 * @param width Width of the window EFB
 * @param height Height of the Window EGB
 * @param xfb_width Width of the XFB
 */
extern void gx_set_copy_window(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t xfb_width);

/**
 * @brief Set the clamp mode when copying from EFB to XFB
 * 
 * Defaults to clamping the top and bottom of the framebuffer.
 * 
 * @param mode
*/
extern void gx_set_clamp_mode(gx_clamp_mode_t mode);

/**
 * @brief Sets the gamma value when copying from EFB to XFB
 * 
 * Defaults to 1.0
 * 
 * @param gamma The gamma value from the enum.
 */
extern void gx_set_gamma(gx_gamma_t gamma);

/**
 * @brief Sets the what lines are copied from EFB to XFB
 * 
 * Used to determine if all lines, or if even and odd lines
 * are copied from EFB to XFB.
 * 
 * It defaults to progressive, aka, all lines.
*/
extern void gx_set_line_mode(gx_line_mode_t line_mode);

/**
 * @brief Sets the interpolation filters used when copying the frame buffer
 * into external frame buffers of differing resolutions.
 * 
 * This is a grid of values that determines how pixels are sampled
 * when copying frame buffer.
 * 
 * For normal video modes you dont need to touch this.
 * 
 * For the X,Y pattern,
 * Each one is between 0-12. The middle of the pixel is 6 (ish).
 * These are for 3x MSAA generation.
 * 
 * filter is the array of ceoffenents between 0 and 63.
 * Thats a Q0:6 fixed point format. And they should sum to 1
 * to keep normal brightness
 * 
 * The first 2 values are the top row.
 * Then the next 3 are the middle row
 * Last 2 are the bottom row.
 * 
 * Standard deflikering pattern is 0.25, 0.5, 0.25.
 * @param pattern Pixel position for the filter.
 * @param filter Ceoffenents for the filter.
 */
extern void gx_set_copy_filter(const uint8_t pattern[12][2], const uint8_t filter[7]);

/**
 * @brief Determins how Z values will be treated.
 * 
 * @param enable_compare Compare Z values? Or just pass all values
 * @param compare Function to compare them with
 * @param enable_update Update Z values of pixels? Or keep the previous.
*/
extern void gx_set_z_mode(bool enable_compare, gx_compare_t compare, bool enable_update);

/** 
 * @brief Enables updating the color when rendering into the EFB
 * 
 * @param update_color Update the color values
 * @param update_alpha Update the alpha values
 */
extern void gx_set_color_update(bool update_color, bool update_alpha);

/**
 * @brief Enables / Disables checking Z before or after texturing.
 *
 * Used to turn off early checking of Z before texturing.
 * Checking Z before texturing allows you to quickly discard the fragment and save
 * bandwidth.
 * 
 * But if the TEV stages or whatever discard the fragment, the Z buffer, during the compare,
 * will be falsy updated when the fragment is discard.
 * 
 * So if your scene contains these. You will need to disable this. Its on by default.
 * 
 * @param enable If true, Z values are checked before texturing.
*/
extern void gx_enable_z_precheck(bool enable);

/**
 * @brief Sets the format of pixels in the EFB.
 * 
 * Sets the format of the pixels in the EFB.
 * 
 * Also lets you control z compression and such.
 * 
 * I will need to comment more on the effects of antialiasing since some modes use it, some dont. 
*/
extern void gx_set_pixel_format(gx_pixel_format_t pixels_format, gx_z_format_t z_format);

/**
 * @brief Copys the internal framebuffer to the external frame buffer.
 * 
 * Copys the internal framebuffer to the external frame buffer.
 * This is usually called during vblank to copy the frame buffer
 * to display the next frame. vsync avoiding the need for double buffering.
 * 
 * @param framebuffer Pointer to frame buffer. Needs to be 32 byte aligned.
 * @param clear Clear the internal frame buffer during the copy.
 */
extern void gx_copy_framebuffer(framebuffer_t* framebuffer, bool clear);