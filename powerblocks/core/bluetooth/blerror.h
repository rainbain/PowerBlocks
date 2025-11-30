/**
 * @file blerror.h
 * @brief Definitions for bluetooth errors.
 *
 * Bluetooth error codes defined.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

// General Errors
#define BLERROR_RUNTIME            -1 // A weird value appeared somewhere
#define BLERROR_IOS_EXCEPTION      -2 // IOS returns an error code
#define BLERROR_FREERTOS           -3 // FreeRTOS gave an error. Likely out of memory.
#define BLERROR_ARGUMENT           -4 // Bad argument is passed
#define BLERROR_HCI_REQUEST_ERR    -5 // HCI Controller returned error
#define BLERROR_HCI_CONNECT_FAILED -6 // Failed to connect to a device.
#define BLERROR_OUT_OF_MEMORY      -7 // System ran out of memory.
#define BLERROR_TIMEOUT            -8

#define BLERROR_L2CAP_SIGNAL_FAILED      -9  // When a signal returns an error.
#define BLERROR_L2CAP_ALREADY_OPEN       -10 // When a device or channel is already open

// BLTools Specific
#define BLERROR_DRIVER_INITIALIZE_FAIL          -13
#define BLERROR_NO_DRIVER_FOUND                 -14