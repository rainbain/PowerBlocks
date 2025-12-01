/**
 * @file debugger.h
 * @brief PowerBlocks Debugger
 *
 * Adds extended debugging features.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

/**
* @brief Installs the debuggers crash hand;er.
*
* Will give a more detailed crash handler than the default
* in line with what the crash handler supports.
*/
extern void debugger_install_crash_handler();