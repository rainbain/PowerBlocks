/**
 * @file log.c
 * @brief System logging functions.
 *
 * Provides implementation of logging to control logging level
 * and where the log goes.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "log.h"

#include <stdio.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "semphr.h"

static SemaphoreHandle_t log_mutex;
static StaticSemaphore_t log_mutex_data;

void log_initialize(void) {
    log_mutex = xSemaphoreCreateMutexStatic(&log_mutex_data);
}

void log_message(const char* level, const char* tag, const char* fmt, ...) {
    xSemaphoreTake(log_mutex, portMAX_DELAY);

    printf("[%s] (%s) ", level, tag);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");

    xSemaphoreGive(log_mutex);
}