/**
 * @file log.h
 * @brief System logging functions.
 *
 * Provides implementation of logging to control logging level
 * and where the log goes.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdio.h>

extern void log_initialize();

extern void log_message(const char* level, const char* tag, const char* fmt, ...);

#define LOG_DEBUG(tab, ...) log_message("DEBUG", tab, ##__VA_ARGS__)
#define LOG_INFO(tab, ...) log_message("INFO", tab, ##__VA_ARGS__)
#define LOG_ERROR(tab, ...) log_message("ERROR", tab, ##__VA_ARGS__)
