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

#include "framebuffer.h"
#include "video.h"

#include "FreeRTOS.h"
#include "task.h"

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

// Some useful defines regarding GX FIFO Spec
#define GX_FIFO_MINIMUM_SIZE (64 * 1024) // Standard small fifo size
#define GX_FIFO_WATERMARK (16 * 1024) // How many bytes to too full and too low level

#include "gx_xf.h"
#include "gx_tev.h"
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

// So in actual hardware, the first 4, and last 4 textures are in
// separated memory addresses. I kept the enum simple though since
// I did not want it messing with things.
typedef enum {
    GX_TEXTURE_ID_0,
    GX_TEXTURE_ID_1,
    GX_TEXTURE_ID_2,
    GX_TEXTURE_ID_3,
    GX_TEXTURE_ID_4,
    GX_TEXTURE_ID_5,
    GX_TEXTURE_ID_6,
    GX_TEXTURE_ID_7
} gx_texture_id_t;

#define GX_WPAR_OPCODE_LOAD_CP                 0x08
#define GX_WPAR_OPCODE_LOAD_XF                 0x10
#define GX_WPAR_OPCODE_LOAD_BP                 0x61
#define GX_WPAR_OPCODE_PRIMITIVE(type, format) (((uint8_t)type) | format)

#define GX_XF_MEMORY_POSTEX_MTX_0     0x0000
#define GX_XF_MEMORY_NORMAL_MTX_0     0x0400
#define GX_XF_MEMORY_DUALTEX_MTX_0    0x0500
#define GX_XF_MEMORY_LIGHT0           0x0600
#define GX_XF_REGISTER_VERTEX_STATS   0x1008
#define GX_XF_REGISTER_COLOR_COUNT    0x1009
#define GX_XF_REGISTER_COLOR_CONTROL  0x100E
#define GX_XF_REGISTER_DUALTEXTRANS   0x1012
#define GX_XF_REGISTER_MTXIDX_A       0x1018
#define GX_XF_REGISTER_MTXIDX_B       0x1019
#define GX_XF_REGISTER_VIEWPORT       0x101A
#define GX_XF_REGISTER_PROJECTION     0x1020
#define GX_XF_REGISTER_TEXCOORD_COUNT 0x103F
#define GX_XF_REGISTER_TEX0           0x1040
#define GX_XF_REGISTER_DUALTEX0       0x1050

#define GX_BP_REGISTERS_GENMODE                      (0x00 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_POS_A            (0x01 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_POS_B            (0x02 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_POS_C            (0x03 << 24)
#define GX_BP_REGISTERS_COPY_FILTER_POS_D            (0x04 << 24)
#define GX_BP_REGISTERS_SCISSOR_TL                   (0x20 << 24)
#define GX_BP_REGISTERS_SCISSOR_BR                   (0x21 << 24)
#define GX_BP_REGISTERS_SSIZE0                       (0x30 << 24)
#define GX_BP_REGISTERS_TSIZE0                       (0x31 << 24)
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
#define GX_BP_REGISTERS_TX_SETMODE0_I0               (0x80 << 24) // 0-3 HERE, 4-7 AT 0XA0
#define GX_BP_REGISTERS_TX_SETMODE1_I0               (0x84 << 24) // 0-3 HERE, 4-7 AT 0XA4
#define GX_BP_REGISTERS_TX_SETIMAGE0_I0              (0x88 << 24) // 0-3 HERE, 4-7 AT 0XA8
#define GX_BP_REGISTERS_TX_SETIMAGE1_I0              (0x8C << 24) // 0-3 HERE, 4-7 AT 0XAC
#define GX_BP_REGISTERS_TX_SETIMAGE2_I0              (0x90 << 24) // 0-3 HERE, 4-7 AT 0XB0
#define GX_BP_REGISTERS_TX_SETIMAGE3_I0              (0x94 << 24) // 0-3 HERE, 4-7 AT 0XB4
#define GX_BP_REGISTERS_TX_SETTLUT_0                 (0x98 << 24) // 0-3 HERE, 4-7 AT 0XB8
#define GX_BP_REGISTERS_TEV0_COLOR_ENV               (0xC0 << 24)
#define GX_BP_REGISTERS_TEV0_ALPHA_ENV               (0xC1 << 24)
#define GX_BP_REGISTERS_TEV_REGISTERL_0              (0xE0 << 24)
#define GX_BP_REGISTERS_TEV_REGISTERH_0              (0xE1 << 24)

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

/** @brief Cool little texture object.
 * Really just a copy of the registers that will be set onto the GPU when the texture is loaded.
*/
typedef struct {
    // Texture Control Registers
    uint32_t mode0; // lookup and filtering register
    uint32_t mode1; // LOD data
    uint32_t image0; // Width, hight, format
    uint32_t image1; // even LOD address in TMEM
    uint32_t image2; // odd LOD address in TMEM
    uint32_t image3; // Address in main memory
    uint32_t lut; // offset + format
} gx_texture_t;

typedef enum {
    GX_TEXTURE_FORMAT_I4,      // 4 Ininstty/Brightness Bits
    GX_TEXTURE_FORMAT_I8,      // 8 Ininstty/Brightness Bits
    GX_TEXTURE_FORMAT_IA4,     // 4 Intensity + 4 Alpha
    GX_TEXTURE_FORMAT_IA8,     // 8 Intensity + 8 Alpha
    GX_TEXTURE_FORMAT_RGB565,  // 5 Red + 6 Green + 5 Blue
    GX_TEXTURE_FORMAT_RGB5A3,  // 5 bits each for R, G, and B, then 3 for alpha
    GX_TEXTURE_FORMAT_RGBA8,   // 8 bits for each R, G, B, and A
    GX_TEXTURE_FORMAT_C4 = 8,  // 4 bit color indexing
    GX_TEXTURE_FORMAT_C8,      // 8 bit color indexing
    GX_TEXTURE_FORMAT_C14X2,   // 14 bit indexing + 2 alignment bits?
    GX_TEXTURE_FORMAT_CMP = 14 // Compressed
} gx_texture_format_t;

typedef enum {
    GX_WRAP_CLAMP,
    GX_WRAP_REPEAT,
    GX_WRAP_MIRROR
} gx_texture_wrap_t;

// This is used internally when decoding texture formats
typedef enum {
    GX_TEXTURE_MIN_FILTER_NEAR,
    GX_TEXTURE_MIN_FILTER_NEAR_MIP,
    GX_TEXTURE_MIN_FILTER_NEAR_MIP_LINEAR,
    GX_TEXTURE_MIN_FILTER_LINEAR = 4,
    GX_TEXTURE_MIN_FILTER_LINEAR_MIP_NEAR,
    GX_TEXTURE_MIN_FILTER_LINEAR_MIP_LINEAR,
} gx_texture_min_filter;

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
extern void gx_fifo_initialize(gx_fifo_t* fifo, void* fifo_buffer, uint32_t fifo_buffer_size);

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
extern void gx_fifo_initialize(gx_fifo_t* fifo, void* fifo_buffer, uint32_t fifo_buffer_size);

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
 * @brief Sets the current render thread for suspension.
 * 
 * When the GX FIFO is too full, it will suspend the graphics
 * thread and resume it once it clears out a bit.
 * 
 * gx_initialize sets this to the current thread. If you plan
 * to call GX functions outside the thread to called gx_initialize.
 * You may want to set it here.
 * 
 * You can also set it to null and bypass this functionality.
 * 
 * @param task Handle to the task.
 */
extern void gx_set_render_thread(TaskHandle_t task);

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

/* -------------------Textures--------------------- */
/**
 * @brief Initializes a texture object for use.
 * 
 * Initializes a texture object to be loaded whenever needed.
 * This does not actually allocate or contain the image data. Make sure to keep that safe.
 * No, it just holds the state of the registers needed in order to draw that texture.
 * 
 * @param texture The texture object to setup
 * @param data Image data of the format
 * @param format Texture Format
 * @param width Width of the texture
 * @param height Height of the texture
 * @param s_wrap S Wrap (X values)
 * @param t_wrap T Wrap (Y values)
 * @param mipmap Enabling Mipmapping 
*/
extern void gx_initialize_texture(gx_texture_t* texture, const void* data, gx_texture_format_t format, int width, int height,
                                  gx_texture_wrap_t s_wrap, gx_texture_wrap_t t_wrap, bool mipmap);

/**
 * @brief Flashes a texture configuration onto the BP
 * 
 * Sets up a texture for drawing by configuring the registers.
 * 
 * Its data wont be loaded into memory at this point, just that future draws
 * with it will begin loading this new data.
 * 
 * @param map The texture channel to load into. 1-8
 * @param texture The texture register data.
 */
extern void gx_flash_texture(gx_texture_map_t map, const gx_texture_t* texture);