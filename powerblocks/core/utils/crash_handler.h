/**
 * @file crash_handler.h
 * @brief Used to display a crash screen for debugging.
 *
 * Provides a simple crash screen to aid with debugging.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

#include "powerblocks/core/system/exceptions.h"

/**
 * @typedef crash_handler_t
 * @brief Function pointer to the system crash handlers.
 *
 * Function pointer to the system crash handlers.
 * Used with crash_handler_set.
 *
 * @param cause The Name / Cause of the Crash
 * @param ctx Execution context. If provided it can display register information and stack at the time.
 */
typedef void (*crash_handler_t)(const char* cause, exception_context_t* ctx);

/**
* @brief Freezes the system and displays a crash screen.
*
* Freezes the system and displays a crash screen.
*
* Calls the set crash handle function, or the default
* 
* The name bug check is a reference to the keBugCheck
* function that displays the BSOD in Windows.
*
* @param cause The Name / Cause of the Crash
* @param ctx Execution context. If provided it can display register information and stack at the time.
*/
extern void crash_handler_bug_check(const char* cause, exception_context_t* ctx);

/**
* @brief Sets the system crash handler.
*
* Sets the system crash handler.
*
* Set to NULL to put it to the default.
*
* Look at the implementation of the default handler for an example of this.
* The handler can return and execution will attempt to continue.
*
* @param handler Function to handle the crash.
*/
extern void crash_handler_set(crash_handler_t handler);