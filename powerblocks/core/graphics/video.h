/**
 * @file video.h
 * @brief Manages the video output of the system.
 *
 * Interacts and initializes the video interface.
 * Has functions for working with VSync, creating and using framebuffers.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>

#include "powerblocks/core/graphics/framebuffer.h"

/**
 * @enum video_mode_t
 * @brief Supported modes for the video interface.
 *
 * This enum is used to selected between the diffent video modes
 * supported by the video interface. More could be supported, but these
 * are the ones programmed into the system.
 */
typedef enum {
    VIDEO_MODE_UNINITIALIZED,
    VIDEO_MODE_640X480_NTSC_INTERLACED,
    VIDEO_MODE_640X480_NTSC_PROGRESSIVE,
    VIDEO_MODE_640X480_PAL50,
    VIDEO_MODE_640X480_PAL60
} video_mode_t;

/**
 * @brief Gets the video mode of the console.
 *
 * Looks at the system settings to get the system video mode.
 * 
 * Must be called AFTER ios_initialize.
 * 
 * This function is unfinished. I need dumps of the other video modes.
 * @return Default video mode.
 */
extern video_mode_t video_system_default_video_mode();

/**
 * @brief Initializes the video output.
 *
 * Initializes the video interface with a given video mode.
 * @param mode Mode for the video interface.
 */
extern void video_initialize(video_mode_t mode);

/**
 * @brief Sets the frame buffer presented to the screen.
 *
 * Sets the frame buffer presented to the screen.
 * 
 * @param framebuffer Framebuffer to put to the screen.
 */
extern void video_set_framebuffer(const framebuffer_t* framebuffer);

/**
 * @brief Gets the frame buffer currently displayed.
 *
 * Gets the frame buffer currently displayed.
 * Can be used for some interesting functionality.
 */
extern framebuffer_t* video_get_framebuffer();

/**
 * @brief Waits for the next vsync with interrupts
 *
 * Waits for the next vsync, waits for the next display interrupt
 * to measure when the display goes into retrace.
 */
extern void video_wait_vsync();

/**
 * @brief Waits for the next vsync without interrupts.
 *
 * Waits for the next vsync, in a interrupt handler safe way.
 * It does by just waiting till the vertical position is in range.
 * 
 * This function seems to have some issues, make sure it actually works.
 * May or may not be an emulator thing.
 */
extern void video_wait_vsync_int();