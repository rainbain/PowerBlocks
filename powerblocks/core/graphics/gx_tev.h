/**
 * @file gx_tev.h
 * @brief TEV definitions.
 *
 * Though the TEV is really part of the BP.
 * The TEV registers have been made their own header.
 * So that their functions are organized.
 * 
 * Diagram of a TEV Stage:
 * 
 * 
 *                                                                                         /|     +------------+
 *                                                                                        / |>----| PREVIOUS   |
 * +---------+  8  +-------------------------+                                           /  |     +------------+
 * | INPUT A |>-/--|                         |                                          /   |
 * +---------+     |                         |                                         |    |     +------------+
 *                 |                         |                                         |    |>----| REGISTER 1 |
 * +---------+  8  |       COLOR MIXER       |     +---------------+     +-------+  11 |    |     +------------+
 * | INPUT B |>-/--|       mix(a, b, c)      |>----| ADD & SCALE   |>----| CLAMP |>-/--|    |
 * +---------+     | (a * (1.0 - c) + b * c) |     | ((c+d+b) * s) |     +-------+     |    |     +------------+
 *                 |                         |     +---------------|                   |    |>----| REGISTER 2 |
 * +---------+  8  |                         |            |                            |    |     +------------+
 * | INPUT C |>-/--|                         |            |                             \   |
 * +---------+     +-------------------------+            |                              \  |     +------------+
 *                                                        |                               \ |>----| REGISTER 3 |
 * +---------+  11                                        |                                \|     +------------+
 * | INPUT D |---/----------------------------------------+
 * +---------+
 * 
 * Notes:
 *   * Each of the inputs, A-D, are selectable.
 *   * Inputs are truncated to 8 bits, except for D that is a signed 11 bits, [-1024, 1023]
 *   * The addition is input D, additional bias, then scale is set to be 0.5, 1, 2, or 4
 *   * If clamping is enabled, color values are capped [0, 255], otherwise [-1024, 1023]
 *   * You can save the output to 1 of 4 TEV registers, or just use the tev previous register.
 *   * TEV previous becomes the pixels color at the end of the TEV pipeline.
 *   * TEVs default mode is biasing mode, it can be in compare mode too. No mixing,
 *       It just says, if operation on input A and B is true, pass C, otherwise, 0. It still adds D as a bias.
 * 
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include <stdint.h>
#include <stdbool.h>

// This header is not supposed to be included directly.
// Instead include gx.h

/* -------------------TEV Stage Control--------------------- */

typedef struct {
    uint32_t color_control;
    uint32_t alpha_control;
} gx_tev_stage_t;

typedef enum {
    GX_TEV_STAGE_0,
    GX_TEV_STAGE_1,
    GX_TEV_STAGE_2,
    GX_TEV_STAGE_3,
    GX_TEV_STAGE_4,
    GX_TEV_STAGE_5,
    GX_TEV_STAGE_6,
    GX_TEV_STAGE_7,
    GX_TEV_STAGE_8,
    GX_TEV_STAGE_9,
    GX_TEV_STAGE_10,
    GX_TEV_STAGE_11,
    GX_TEV_STAGE_12,
    GX_TEV_STAGE_13,
    GX_TEV_STAGE_14,
    GX_TEV_STAGE_15
} gx_tev_stage_id;

typedef enum {
    GX_TEV_IO_PREVIOUS   = 0x0,
    GX_TEV_IO_REGISTER_0 = 0x2,
    GX_TEV_IO_REGISTER_1 = 0x4,
    GX_TEV_IO_REGISTER_2 = 0x6,
    GX_TEV_IO_TEXTURE    = 0x8,
    GX_TEV_IO_RASTERIZER = 0xA,
    GX_TEV_IO_ONE        = 0xC, // Unsupported as alpha input
    GX_TEV_IO_HALF       = 0xD, // Unsupported as alpha input
    GX_TEV_IO_CONSTANT   = 0xE,
    GX_TEV_IO_ZERO       = 0xF,

    GX_TEV_IO_ALPHA      = 0x1, // Use alpha channel for color input
} gx_tev_io_t;

typedef enum {
    GX_TEV_SCALE_1,
    GX_TEV_SCALE_2,
    GX_TEV_SCALE_4,
    GX_TEV_SCALE_HALF
} gx_tev_scale_t;

typedef enum {
    GX_TEV_BIAS_0,
    GX_TEV_BIAS_ADD_HALF,
    GX_TEV_BIAS_SUB_HALF
} gx_tev_bias_t;

typedef enum {
    GX_TEV_COMPARE_R8_GREATER    = 0x0,
    GX_TEV_COMPARE_R8_EQUAL      = 0x1,
    GX_TEV_COMPARE_GR16_GREATER  = 0x2,
    GX_TEV_COMPARE_GR16_EQUAL    = 0x3,
    GX_TEV_COMPARE_BGR24_GREATER = 0x4,
    GX_TEV_COMPARE_BGR24_EQUAL   = 0x5,
    GX_TEV_COMPARE_RGB8_GT       = 0x6,
    GX_TEV_COMPARE_RGB8_EQ       = 0x7,
    GX_TEV_COMPARE_A8_GREATER    = 0x6,
    GX_TEV_COMPARE_A8_EQUAL      = 0x7
} gx_tev_compare_t;

/**
 * @brief Sets the number of tev stages.
 * 
 * Sets the number of TEV stages the hardware will process.
 * 
 * @param count TEV Stage count 1 to 16
 */
extern void gx_set_tev_stages(int count);

/**
 * @brief Generates a empty TEV stage.
 * 
 * It will just pass rasterizer/color value.
 * 
 * @param tev TEV stage to set.
*/
extern void gx_initialize_tev_stage(gx_tev_stage_t* tev);

/**
 * @brief Flashes a TEV stage onto the BP
 *
 * Copys TEV stages onto the BP.
 * 
 * @param id TEV stage id.
 * @param tev Pointer to TEV stage.
 */
extern void gx_flash_tev_stage(gx_tev_stage_id id, const gx_tev_stage_t* tev);

/**
 * @brief Sets the color inputs to a TEV stage
 * 
 * Sets the color inputs to a TEV stage
 * 
 * Setting the GX_TEV_INPUT_ALPHA bit on a input,
 * will instead use the alpha channel for that input.
 * 
 * @param tev TEV stage to modify.
 * @param input_a Input A's RGB8 value
 * @param input_b Input B's RGB8 value
 * @param input_c Input C's RGB8 value
 * @param input_d Input D's 11 bit signed value
*/
extern void gx_set_tev_stage_color_input(gx_tev_stage_t* tev, gx_tev_io_t a, gx_tev_io_t b, gx_tev_io_t c, gx_tev_io_t d);

/**
 * @brief Sets the color inputs to a TEV stage
 * 
 * Sets the alpha inputs to a TEV stage
 * 
 * In this you can not use GX_TEV_INPUT_ONE and GX_TEV_INPUT_HALF.
 * 
 * GX_TEV_INPUT_ALPHA will not do anything.
 * 
 * @param tev TEV stage to modify.
 * @param input_a Input A's alpha value
 * @param input_b Input B's alpha value
 * @param input_c Input C's alpha value
 * @param input_d Input D's 11 bit signed value
*/
extern void gx_set_tev_stage_alpha_input(gx_tev_stage_t* tev, gx_tev_io_t a, gx_tev_io_t b, gx_tev_io_t c, gx_tev_io_t d);

/**
 * @brief Sets the color output to a TEV stage
 * 
 * Sets the color output to a TEV stage
 * 
 * @param tev TEV stage to modify.
 * @param out Output Destination
 * @param clamp Clamp the output [0, 255] or else [-1024, 1023]
 */
extern void gx_set_tev_stage_color_output(gx_tev_stage_t* tev, gx_tev_io_t out, bool clamp);

/**
 * @brief Sets the alpha output to a TEV stage
 * 
 * Sets the alpha output to a TEV stage
 * 
 * @param tev TEV stage to modify.
 * @param out Output Destination
 * @param clamp Clamp the output [0, 255] or else [-1024, 1023]
 */
extern void gx_set_tev_stage_alpha_output(gx_tev_stage_t* tev, gx_tev_io_t out, bool clamp);

/**
 * @brief Sets the TEV color stage into biasing mode.
 * 
 * In biasing mode, the formula is like `(mix(a, b, c) +- d + bias) * scale`
 * This in contrary to compare mode where the formula is `d + (a compare b ? c : 0)`
 * 
 * @param tev TEV stage to modify.
 * @param subtract If set, the d value will be subtracted
 * @param bias Bias value, -0.5, 0, 0.5.
 * @param scale Scale. 0.5, 1, 2, or 4
 */
extern void gx_set_tev_stage_color_biasing(gx_tev_stage_t* tev, bool subtract, gx_tev_bias_t bias, gx_tev_scale_t scale);

/**
 * @brief Sets the TEV color stage into comparison mode.
 * 
 * In biasing mode, the formula is like `(mix(a, b, c) +- d + bias) * scale`
 * This in contrary to compare mode where the formula is `d + (a compare b ? c : 0)`
 * 
 * @param tev TEV stage to modify.
 * @param comparison Comparison mode to use.
 */
extern void gx_set_tev_stage_color_comparison(gx_tev_stage_t* tev, gx_tev_compare_t comparison);

/**
 * @brief Sets the TEV alpha stage into biasing mode.
 * 
 * In biasing mode, the formula is like `(mix(a, b, c) +- d + bias) * scale`
 * This in contrary to compare mode where the formula is `d + (a compare b ? c : 0)`
 * 
 * @param tev TEV stage to modify.
 * @param subtract If set, the d value will be subtracted
 * @param bias Bias value, -0.5, 0, 0.5.
 * @param scale Scale. 0.5, 1, 2, or 4
 */
extern void gx_set_tev_stage_alpha_biasing(gx_tev_stage_t* tev, bool subtract, gx_tev_bias_t bias, gx_tev_scale_t scale);

/**
 * @brief Sets the TEV alpha stage into comparison mode.
 * 
 * In biasing mode, the formula is like `(mix(a, b, c) +- d + bias) * scale`
 * This in contrary to compare mode where the formula is `d + (a compare b ? c : 0)
 * 
 * @param tev TEV stage to modify.
 * @param comparison Comparison mode to use.
 */
extern void gx_set_tev_stage_alpha_comparison(gx_tev_stage_t* tev, gx_tev_compare_t comparison);

/* -------------------TEV Registers --------------------- */

/**
 * @brief Set the value of one of the TEVs internal registers
 * 
 * The TEV's internal registers can be used to store constant colors if wanted.
 * Only GX_TEV_IO_REGISTER_0, GX_TEV_IO_REGISTER_1, and GX_TEV_IO_REGISTER_2 are valid. 
 * 
 * @param color Register to put it into.
 * @param r Red value [0, 255], or a signed 11 bit value [-1024, 1024]
 * @param g Green value [0, 255], or a signed 11 bit value [-1024, 1024]
 * @param b Blue value [0, 255], or a signed 11 bit value [-1024, 1024]
 * @param a Alpha value [0, 255], or a signed 11 bit value [-1024, 1024]
*/
extern void gx_flash_tev_register_color(gx_tev_io_t reg, int r, int g, int b, int a);