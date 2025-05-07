/**
 * @file libcio.c
 * @brief Adds standard IO to libc.
 *
 * Adds standard IO to libc.
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include <stdio.h>

#include "utils/console.h"

static int libc_put(char c, FILE* file) {
    char str[] = {c, 0};
    console_put(str);
    return 1;
}

static int libc_get(FILE* file) {
    return 0;
}

static int libc_flush(FILE* file) {
    return 0;
}

FILE static_stdout = {
    .unget = 0,
    .flags = __SRD | __SWR, 

    .put = libc_put,
    .get = libc_get,
    .flush = libc_flush
};

FILE *const stdout = &static_stdout;