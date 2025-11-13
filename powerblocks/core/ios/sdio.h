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

#pragma once

#include <stdint.h>
#include <stddef.h>

// Implementation based on https://wiibrew.org/wiki//dev/sdio.

/**
 * @brief Standard SD Commands
 * 
 * This list contains the standard SD commands + SDIO specific commands.
 * This list does not contain all of them, just the common ones.
 */
typedef enum {
    SD_CMD0_GO_IDLE_STATE         = 0,
    SD_CMD2_ALL_SEND_CID          = 2,
    SD_CMD3_SEND_RELATIVE_ADDR    = 3,
    SD_CMD7_SELECT_CARD           = 7,
    SD_CMD8_SEND_IF_COND          = 8,
    SD_CMD9_SEND_CSD              = 9,
    SD_CMD12_STOP_TRANSMISSION    = 12,
    SD_CMD16_SET_BLOCKLEN         = 16,
    SD_CMD17_READ_SINGLE_BLOCK    = 17,
    SD_CMD18_READ_MULTIPLE_BLOCK  = 18,
    SD_CMD24_WRITE_BLOCK          = 24,
    SD_CMD25_WRITE_MULTIPLE_BLOCK = 25,
    SD_CMD55_APP_CMD              = 55,
    SD_CMD56_GEN_CMD              = 56,

    SDIO_CMD5_IO_SEND_OP_COND     = 5,
    SDIO_CMD52_IO_RW_DIRECT       = 52,
    SDIO_CMD53_IO_RW_EXTENDED     = 53,

    SDIO_ACMD6_SET_BUS_WIDTH      = 6
} sdio_cmd_t;

/**
 * @brief Standard SD Command Types
 */
typedef enum {
    SD_CMDTYPE_BC   = 1,  // Broadcast
    SD_CMDTYPE_BCR  = 2,  // Broadcast with response
    SD_CMDTYPE_AC   = 3,  // Addressed, no data
    SD_CMDTYPE_ADTC = 4   // Addressed, data transfer
} sdio_cmdtype_t;

/**
 * @brief Standard SD Response Types
 */
typedef enum {
    SD_RESP_NONE = 0,  // No response
    SD_RESP_R1,        // Normal command response
    SD_RESP_R1b,       // With busy signal
    SD_RESP_R2,        // CID/CSD response
    SD_RESP_R3,        // OCR response
    SD_RESP_R4,        // IO_SEND_OP_COND response
    SD_RESP_R5,        // IO_RW_DIRECT response
    SD_RESP_R6,        // Published RCA response
    SD_RESP_R7         // Interface condition
} sdio_resptype_t;

#define SDIO_DEVICE_STATUS_CARD_INSERTED        (1<<0)
#define SDIO_DEVICE_STATUS_NOT_INSERTED         (1<<1)
#define SDIO_DEVICE_STATUS_WRITE_PROTECT_SWITCH (1<<2)
#define SDIO_DEVICE_STATUS_SD_INITIALIZED       (1<<16)
#define SDIO_DEVICE_STATUS_IS_SDHC              (1<<20)

/**
 * @brief Opens the SDIO interface
 * 
 * Opens the SDIO interface on the wii.
 * 
 * @param device SDIO device file to open, usually /dev/sdio/slot0 (front SD)
 * 
 * @return Negative if Error
 */
extern int sdio_initialize(const char* device);

/**
 * @brief =Closes the driver.
 * 
 * Closes the driver and assisted IOS file handle.
 */
extern void sdio_close();

/**
 * @brief Returns the status of the SD card slot.
 * 
 * @param state Outputted uint32_t state bitfield
 * @return Negative if error.
 */
extern int sdio_get_device_status(uint32_t *state);

/**
 * @brief Resets the SDIO device, and returns RCA.
 * 
 * @param response First 16 bits are the RCA, next 16 bits are stuff bits. Following the SDIO CMD3.
 * @return Negative if error
 */
extern int sdio_reset_device(uint32_t* response);

/**
 * @brief Sets the clock source for the SD card
 * 
 * @param divider Power of two, or zero, usually one.
 * @return Negative if error
 */
extern int sdio_set_clock(uint32_t divider);

/**
 * @brief Reads the OC register
 * Reads the value of the SD cards OC register.
 * This contains a bit feild of operating conditions.
 * 
 * @param oc Outputted 32 bit bit feild
 * @return Negative if error.
 */
extern int sdio_read_oc_register(uint32_t* oc);

/**
 * @brief Reads a host controller register.
 * The wii uses a standard SDIO host controller. You may read an
 * arbitrary register of it using this.
 * 
 * @param offset Register offset.
 * @param value Outputted register value
 * @return Negative if error.
 */
extern int sdio_read_hc_register(uint32_t offset, uint32_t* value);


/**
 * @brief Sets a host controller register.
 * The wii uses a standard SDIO host controller. You may set an
 * arbitrary register of it using this.
 * 
 * @param offset Register offset.
 * @param value Outputted register value
 * @return Negative if error.
 */
extern int sdio_set_hc_register(uint32_t offset, uint32_t value);

/**
 * @brief Sends a SD card command.
 * Sends a command and returns a 16 byte response.
 * 
 * @param cmd             What command for the card to carry out.
 * @param cmd_type        What is contained within the command, what to do.
 * @param resp_type       How to interpret the cards response?
 * @param arg             Argument specific to the command.
 * @param data            For commands with associated data, or NULL
 * @param block_count     How many blocks in the data
 * @param sector_size     Size of a block in 
 * @param response        Pointer to put the SD response in
 * @param response_length Length of the response
 */
extern int sdio_send_cmd(sdio_cmd_t cmd, sdio_cmdtype_t type, sdio_resptype_t resptype, uint32_t arg, void* data, uint32_t block_count, uint32_t sector_size, void* response, size_t response_length);