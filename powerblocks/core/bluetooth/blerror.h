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
#define BLERROR_RUNTIME         -1 // A weird value appeared somewhere
#define BLERROR_IOS_EXCEPTION   -2 // IOS returns an error code
#define BLERROR_FREERTOS        -3 // FreeRTOS gave an error. Likely out of memory.
#define BLERROR_ARGUMENT        -4 // Bad argument is passed
#define BLERROR_HCI_REQUEST_ERR -5 // HCI Controller returned error
#define BLERROR_OUT_OF_MEMORY   -6 // System ran out of memory.

// Specific to opening a channel on L2CAP.
#define BLERROR_CONNECTION_REFUSED_BAD_PSM      -7
#define BLERROR_CONNECTION_REFUSED_SECURITY     -8
#define BLERROR_CONNECTION_REFUSED_NO_RESOURCED -9
#define BLERROR_CONNECTION_REFUSED_UNKNOWN      -10

// BLTools Specific
#define BLERROR_DRIVER_INITIALIZE_FAIL          -11
#define BLERROR_NO_DRIVER_FOUND                 -12