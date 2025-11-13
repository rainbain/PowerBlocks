/**
 * @file sd.c
 * @brief SD Card System
 *
 * Implements the Wii's SD Card Slot
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */
#include "sd.h"

#include "powerblocks/core/ios/sdio.h"
#include "powerblocks/core/utils/log.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>

static const char* TAG = "SD";

#define SD_ERROR_LOGGING
#define SD_INFO_LOGGING
#define SD_DEBUG_LOGGING

#ifdef SD_ERROR_LOGGING
#define SD_LOG_ERROR(fmt, ...) LOG_ERROR(TAG, fmt, ##__VA_ARGS__)
#else
#define SD_LOG_ERROR(fmt, ...)
#endif 

#ifdef SD_INFO_LOGGING
#define SD_LOG_INFO(fmt, ...) LOG_INFO(TAG, fmt, ##__VA_ARGS__)
#else
#define SD_LOG_INFO(fmt, ...)
#endif 

#ifdef SD_DEBUG_LOGGING
#define SD_LOG_DEBUG(fmt, ...) LOG_DEBUG(TAG, fmt, ##__VA_ARGS__)
#else
#define SD_LOG_DEBUG(fmt, ...)
#endif

static bool sd_initialized = false;
static uint32_t sd_rca;
static bool sd_card_sdhc;

int sd_initialize() {
    // Open front SD slot
    int ret = sdio_initialize("/dev/sdio/slot0");

    if(ret < 0) {
        return ret;
    }

    sd_initialized = true;

    return ret;
}

void sd_close() {
    sdio_close();
    sd_initialized = false;
}

DSTATUS sd_disk_status() {
    DSTATUS disk_status = 0;

    if(!sd_initialized) {
        return STA_NOINIT;
    }

    // Check the state of the sd card
    uint32_t status;
    int ret;
    ret = sdio_get_device_status(&status);

    if(ret < 0) {
        SD_LOG_ERROR("Failed to get device status: %d", ret);
        return STA_NOINIT;
    }

    // Save if this is a SDHC card, we need that
    sd_card_sdhc = (status & SDIO_DEVICE_STATUS_IS_SDHC) > 0;

    // Kind of arbitrary phrasing of the status here.
    if((status & SDIO_DEVICE_STATUS_NOT_INSERTED) || !(status & SDIO_DEVICE_STATUS_CARD_INSERTED)) {
        disk_status |= STA_NODISK;
    }

    if(status & SDIO_DEVICE_STATUS_WRITE_PROTECT_SWITCH) {
        disk_status |= STA_PROTECT;
    }

    return disk_status;
}

DSTATUS sd_disk_initialize() {
    int ret;

    SD_LOG_INFO("Mounting SD card.");

    // Reset card and get the device address
    ret = sdio_reset_device(&sd_rca);
    if(ret < 0) {
        SD_LOG_ERROR("Failed to reset card: %d", ret);
        return  STA_NOINIT;
    }

    // Remove stuff bits and single out RCA
    sd_rca &= 0xFFFF0000;

    // Wait until the card is ready.
    uint32_t ocr;
    for(int i = 0; i < 100; i++) {
        ret = sdio_read_oc_register(&ocr);
        if(ret < 0) {
            SD_LOG_ERROR("Failed to read OCR: %d", ret);
            return STA_NOINIT;
        }

        // Ready bit
        if(ocr & (1<<31))
            break;

        // Keep waiting
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // If were never ready, thats one mean card
    if(!(ocr & (1<<31))) {
        SD_LOG_ERROR("Card never ready!");
        return STA_NOINIT;
    }

    // See if its sdhc and get status
    DSTATUS status = sd_disk_status();
    SD_LOG_DEBUG("SD Status:");
    SD_LOG_DEBUG("  Card Inserted:     %s", (status & SDIO_DEVICE_STATUS_CARD_INSERTED) ? "True" : "False");
    SD_LOG_DEBUG("  Card Not Inserted: %s", (status & SDIO_DEVICE_STATUS_NOT_INSERTED) ? "True" : "False");
    SD_LOG_DEBUG("  Write Protected:   %s", (status & SDIO_DEVICE_STATUS_WRITE_PROTECT_SWITCH) ? "True" : "False");
    SD_LOG_DEBUG("  SD Initialized:    %s", (status & SDIO_DEVICE_STATUS_SD_INITIALIZED) ? "True" : "False");
    SD_LOG_DEBUG("  IS SDHC:           %s", (status & SDIO_DEVICE_STATUS_IS_SDHC) ? "True" : "False");

    // Select the card
    ret = sdio_send_cmd(SD_CMD7_SELECT_CARD, SD_CMDTYPE_AC, SD_RESP_R1b, sd_rca, NULL, 0, 0, NULL, 0);
    if(ret < 0) {
        SD_LOG_ERROR("cmd7 failed");
        return status | STA_NOINIT;
    }

    // Set block type, only needed for non SDHC
    if(!sd_card_sdhc) {
        ret = sdio_send_cmd(SD_CMD16_SET_BLOCKLEN, SD_CMDTYPE_AC, SD_RESP_R1, 0x200, NULL, 0, 0, NULL, 0);
        if(ret < 0) {
            SD_LOG_ERROR("cmd16 failed");
            return status | STA_NOINIT;
        }
    }

    // Bus width = 4 bits
    ret = sdio_send_cmd(SD_CMD55_APP_CMD, SD_CMDTYPE_AC, SD_RESP_R1, sd_rca, NULL, 0, 0, NULL, 0);
    if(ret < 0) {
        SD_LOG_ERROR("cmd55 failed");
        return status | STA_NOINIT;
    }
    ret = sdio_send_cmd(SDIO_ACMD6_SET_BUS_WIDTH, SD_CMDTYPE_AC, SD_RESP_R1, 2, NULL, 0, 0, NULL, 0);
    if(ret < 0) {
       SD_LOG_ERROR("acmd6 failed");
       return status | STA_NOINIT;
    }

    // Set the 4 bits on the host controller too
    uint32_t hcr;
    ret = sdio_read_hc_register(0x28, &hcr);
    if(ret < 0) {
       SD_LOG_ERROR("read_hc failed");
       return status | STA_NOINIT;
    }

    hcr |= 0x02;

    ret = sdio_set_hc_register(0x28, hcr);
    if(ret < 0) {
       SD_LOG_ERROR("set_hc failed");
       return status | STA_NOINIT;
    }

    // Now set the clock speed
    ret = sdio_set_clock(1);
    if(ret < 0) {
        SD_LOG_ERROR("set_clock failed");
        return status | STA_NOINIT;
    }

    // And that should be it
    return status;
}

DRESULT sd_disk_read(BYTE* buff, LBA_t sector, UINT count) {
    int ret = sdio_send_cmd(SD_CMD18_READ_MULTIPLE_BLOCK, SD_CMDTYPE_AC, SD_RESP_R1, sector * (sd_card_sdhc ? 1 : 512), buff, count, 512, NULL, 0);
    system_invalidate_dcache(buff, count * 512);
    if(ret < 0) {
        SD_LOG_ERROR("Disk read failed on sector: %d", sector);
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT sd_disk_write(const BYTE* buff, LBA_t sector, UINT count) {
    system_flush_dcache(buff, count * 512);
    int ret = sdio_send_cmd(SD_CMD25_WRITE_MULTIPLE_BLOCK, SD_CMDTYPE_AC, SD_RESP_R1, sector * (sd_card_sdhc ? 1 : 512), (void*)buff, count, 512, NULL, 0);
    if(ret < 0) {
        SD_LOG_ERROR("Disk write failed on sector: %d", sector);
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT sd_disk_ioctl(BYTE cmd, void* buff) {
    switch (cmd) {
        // IOS / the controller handles this
        case(CTRL_SYNC):
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            return RES_OK;
        
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            return RES_OK;
        
        // Not supported
        default:
            SD_LOG_ERROR("Unsupported FatFS IOCTL %d", cmd);
            return RES_PARERR;
    }
}