/**
 * @file sdio.h
 * @brief IOS SDIO Interface
 *
 * The SD card slot on the wii goes to a SDIO
 * interface accessible through IOS.
 * 
 * This driver is to interface with it.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

 
#include "sdio.h"

#include "ios.h"

#include "utils/log.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

static const char* TAG = "SDIO";

#define SDIO_ERROR_LOGGING
//#define SDIO_INFO_LOGGING
//#define SDIO_DEBUG_LOGGING

#ifdef SDIO_ERROR_LOGGING
#define SDIO_LOG_ERROR(fmt, ...) LOG_ERROR(TAG, fmt, ##__VA_ARGS__)
#else
#define SDIO_LOG_ERROR(fmt, ...)
#endif 

#ifdef SDIO_INFO_LOGGING
#define SDIO_LOG_INFO(fmt, ...) LOG_INFO(TAG, fmt, ##__VA_ARGS__)
#else
#define SDIO_LOG_INFO(fmt, ...)
#endif 

#ifdef SDIO_DEBUG_LOGGING
#define SDIO_LOG_DEBUG(fmt, ...) LOG_DEBUG(TAG, fmt, ##__VA_ARGS__)
#else
#define SDIO_LOG_DEBUG(fmt, ...)
#endif

#define SDIO_IOCTL_SET_HC_REGISTER   0x01
#define SDIO_IOCTL_READ_HC_REGISTER  0x02
#define SDIO_IOCTL_RESET_DEVICE      0x04
#define SDIO_IOCTL_GET_DEVICE_STATUS 0x0B
#define SDIO_IOCTL_READ_OC_REGISTER  0x0C

static struct {
    int file;

    SemaphoreHandle_t lock;

    StaticSemaphore_t semaphore_data[1];
} sdio_state;

#define LOCK() xSemaphoreTake(sdio_state.lock, portMAX_DELAY)
#define UNLOCK() xSemaphoreGive(sdio_state.lock)

int sdio_initialize(const char* device) {
    int ret;

    sdio_state.file = ios_open(device, IOS_MODE_READ | IOS_MODE_WRITE);

    if(sdio_state.file < 0) {
        SDIO_LOG_ERROR("Error opening \"%s\": %d", device, sdio_state.file);
        return sdio_state.file;
    }

    sdio_state.lock = xSemaphoreCreateMutexStatic(&sdio_state.semaphore_data[0]);

    return 0;
}

void sdio_close() {
    if(sdio_state.file < 0)
        return;

    LOCK();

    ios_close(sdio_state.file);
    sdio_state.file = -1;

    UNLOCK();
}

int sdio_get_device_status(uint32_t *state) {
    int ret;

    LOCK();
    ret = ios_ioctl(sdio_state.file, SDIO_IOCTL_GET_DEVICE_STATUS, NULL, 0, (void*)state, sizeof(*state));
    UNLOCK();

    return ret;
}

int sdio_reset_device(uint32_t* response) {
    int ret;

    LOCK();
    ret = ios_ioctl(sdio_state.file, SDIO_IOCTL_RESET_DEVICE, NULL, 0, (void*)response, sizeof(*response));
    UNLOCK();

    return ret;
}

int sdio_read_oc_register(uint32_t* oc) {
    int ret;

    LOCK();
    ret = ios_ioctl(sdio_state.file, SDIO_IOCTL_READ_OC_REGISTER, NULL, 0, oc, sizeof(*oc));
    UNLOCK();

    return ret;
}

int sdio_read_hc_register(uint32_t offset, uint32_t* value) {
    int ret;

    // A rather odd setup taken from wiibrew's documentation
    uint32_t input[6];
    memset(input, 0, sizeof(input));
    input[0] = offset;
    input[3] = 1;

    LOCK();
    ret = ios_ioctl(sdio_state.file, SDIO_IOCTL_READ_HC_REGISTER, input, sizeof(input), (void*)value, sizeof(*value));
    UNLOCK();

    return ret;
}


int sdio_set_hc_register(uint32_t offset, uint32_t value) {
    int ret;

    // A rather odd setup taken from wiibrew's documentation
    uint32_t input[6];
    memset(input, 0, sizeof(input));
    input[0] = offset;
    input[3] = 1;
    input[4] = value;

    // FIX ME
    // FIX ME NOW
    // DONT FORGET TO FIX ME
    LOCK();
    ret = ios_ioctl(sdio_state.file, SDIO_IOCTL_SET_HC_REGISTER, input, sizeof(input), (void*)&value, sizeof(value));
    UNLOCK();

    return ret;
}