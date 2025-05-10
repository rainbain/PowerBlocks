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
* @brief Freezes the system and displays a crash screen.
*
* Freezes the system and displays a crash screen.
* 
* The name bug check is a reference to the keBugCheck
* function that displays the BSOD in Windows.
*
* @param cause - The Name / Cause of the Crash
* @param ctx - Execution context. If provided it can display register information and stack at the time.
*/

extern void crash_handler_bug_check(const char* cause, exception_context_t* ctx);