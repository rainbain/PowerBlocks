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

#define LOG_DEBUG(tab, fmt, ...) printf("[DEBUG] (%s) " fmt "\n", tab, ##__VA_ARGS__)
#define LOG_INFO(tab, fmt, ...) printf("[INFO] (%s) " fmt "\n", tab, ##__VA_ARGS__)
#define LOG_ERROR(tab, fmt, ...) printf("[ERROR] (%s) " fmt "\n", tab, ##__VA_ARGS__)