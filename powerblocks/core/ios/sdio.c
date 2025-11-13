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
#define SDIO_IOCTL_SET_CLOCK         0x06
#define SDIO_IOCTL_SEND_CMD          0x07
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

int sdio_set_clock(uint32_t divider) {
    int ret;

    LOCK();
    ret = ios_ioctl(sdio_state.file, SDIO_IOCTL_SET_CLOCK, &divider, sizeof(divider), NULL, 0);
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

    LOCK();
    ret = ios_ioctl(sdio_state.file, SDIO_IOCTL_SET_HC_REGISTER, input, sizeof(input), NULL, 0);
    UNLOCK();

    return ret;
}

#include <stdalign.h>

int sdio_send_cmd(sdio_cmd_t cmd, sdio_cmdtype_t type, sdio_resptype_t resptype, uint32_t arg, void* data, uint32_t block_count, uint32_t sector_size, void* response, size_t response_length) {
    int ret;
    // Fixed size buffer for the ioctl.
    uint8_t response_buffer[16];

    // Setup our request
    uint32_t input[9];
    input[0] = cmd;
    input[1] = type;
    input[2] = resptype;
    input[3] = arg;
    input[4] = block_count;
    input[5] = sector_size;
    input[6] = (uint32_t)data;
    input[8] = 0;


    // So for reasons, not know to me
    // You need IOCTL for non-DMA stuff
    // And IOCTLV if you do DMA.
    if(data == NULL) {
        // No DMA
        input[7] = 0; // Do we need to DMA?
        
        // A rather odd setup taken from wiibrew's documentation

        LOCK();
        ret = ios_ioctl(sdio_state.file, SDIO_IOCTL_SEND_CMD, input, sizeof(input), response_buffer, sizeof(response_buffer));
        UNLOCK();
    } else {
        // Yes DMA
        input[7] = 1; // Do we need to DMA?

        ios_ioctlv_t vectors[3];
        vectors[0].data = input;
        vectors[0].size = sizeof(uint32_t) * 9;

        vectors[1].data = data;
        vectors[1].size = block_count * sector_size;


        vectors[2].data = response_buffer;
        vectors[2].size = sizeof(response_buffer);

        LOCK();
        ret = ios_ioctlv(sdio_state.file, SDIO_IOCTL_SEND_CMD, 2, 1, vectors);
        UNLOCK();
    }

    // Copy back the response
    memcpy(response, response_buffer, response_length);

    return ret;
}
