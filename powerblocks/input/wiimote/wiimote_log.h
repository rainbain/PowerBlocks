/**
 * @file wiimote_log.h
 * @brief Shared logging functions.
 *
 * Shared logging functions across some of the wiimote source files.
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

#include "powerblocks/core/utils/log.h"

extern const char* WIIMOTE_TAG;

#define WIIMOTE_ERROR_LOGGING
#define WIIMOTE_INFO_LOGGING
#define WIIMOTE_DEBUG_LOGGING

#ifdef WIIMOTE_ERROR_LOGGING
#define WIIMOTE_LOG_ERROR(fmt, ...) LOG_ERROR(WIIMOTE_TAG, fmt, ##__VA_ARGS__)
#else
#define WIIMOTE_LOG_ERROR(fmt, ...)
#endif 

#ifdef WIIMOTE_INFO_LOGGING
#define WIIMOTE_LOG_INFO(fmt, ...) LOG_INFO(WIIMOTE_TAG, fmt, ##__VA_ARGS__)
#else
#define WIIMOTE_LOG_INFO(fmt, ...)
#endif 

#ifdef WIIMOTE_DEBUG_LOGGING
#define WIIMOTE_LOG_DEBUG(fmt, ...) LOG_DEBUG(WIIMOTE_TAG, fmt, ##__VA_ARGS__)
#else
#define WIIMOTE_LOG_DEBUG(fmt, ...)
#endif