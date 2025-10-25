/**
 * @file hci.c
 * @brief Bluetooth HCI Driver
 *
 * This driver talks to USB HCI devices.
 * Those are "Host Controller Interfaces"
 * for communicating with bluetooth devices over USB
 * 
 * This is done as Wiimotes are connected through the Wii's
 * internal bluetooth dongle.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#include "hci.h"

#include "powerblocks/core/system/system.h"
#include "powerblocks/core/ios/ios.h"

#include "powerblocks/core/utils/log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "blerror.h"

#include <string.h>
#include <stdalign.h>
#include <endian.h>

// It was not well said if this has to be 32 bytes, so its been included to be changeable.
#define USB_ALIGN 32

#define HCI_TASK_STACK_SIZE  8192
#define HCI_TASK_PRIORITY    (configMAX_PRIORITIES / 4 * 3)

#define HCI_ERROR_LOGGING
//#define HCI_INFO_LOGGING
//#define HCI_DEBUG_LOGGING

#ifdef HCI_ERROR_LOGGING
#define HCI_LOG_ERROR(tab, fmt, ...) LOG_ERROR(tab, fmt, ##__VA_ARGS__)
#else
#define HCI_LOG_ERROR(tab, fmt, ...)
#endif 

#ifdef HCI_INFO_LOGGING
#define HCI_LOG_INFO(tab, fmt, ...) LOG_INFO(tab, fmt, ##__VA_ARGS__)
#else
#define HCI_LOG_INFO(tab, fmt, ...)
#endif 

#ifdef HCI_DEBUG_LOGGING
#define HCI_LOG_DEBUG(tab, fmt, ...) LOG_DEBUG(tab, fmt, ##__VA_ARGS__)
#else
#define HCI_LOG_DEBUG(tab, fmt, ...)
#endif

// This is the event mask bitfield as of Bluetooth V2.1
// Some may not be supported by the hardware
typedef enum {
    HCI_EVENT_ID_INQUIRY_COMPLETE                         = 0,
    HCI_EVENT_ID_INQUIRY_RESULT                           = 1,
    HCI_EVENT_ID_CONNECTION_COMPLETE                      = 2,
    HCI_EVENT_ID_CONNECTION_REQUEST                       = 3,
    HCI_EVENT_ID_DISCONNECTION_COMPLETE                   = 4,
    HCI_EVENT_ID_AUTHENTICATION_COMPLETE                  = 5,
    HCI_EVENT_ID_REMOTE_NAME_REQUEST                      = 6,
    HCI_EVENT_ID_ENCRYPTION_CHANGE                        = 7, 
    HCI_EVENT_ID_CHANGE_CONNECTION_LINK_KEY_COMPLETE      = 8,
    HCI_EVENT_ID_MASTER_LINK_KEY_COMPLETE                 = 9,
    HCI_EVENT_ID_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE  = 10,
    HCI_EVENT_ID_READ_REMOTE_VERSION_INFORMATION_COMPLETE = 11,
    HCI_EVENT_ID_QOS_SETUP_COMPLETE                       = 12,
    HCI_EVENT_ID_HARDWARE_ERROR                           = 15,
    HCI_EVENT_ID_FLUSH_OCCURRED                           = 16,
    HCI_EVENT_ID_ROLE_CHANGE                              = 17,
    HCI_EVENT_ID_MODE_CHANGE                              = 19,
    HCI_EVENT_ID_RETURN_LINK_KEYS                         = 20,
    HCI_EVENT_ID_PIN_CODE_REQUEST                         = 21,
    HCI_EVENT_ID_LINK_KEY_REQUEST                         = 22,
    HCI_EVENT_ID_LINK_KEY_NOTIFICATION                    = 23,
    HCI_EVENT_ID_LOOPBACK_COMMAND                         = 24,
    HCI_EVENT_ID_DATA_BUFFER_OVERFLOW                     = 25,
    HCI_EVENT_ID_MAX_SLOTS_CHANGE                         = 26,
    HCI_EVENT_ID_READ_CLOCK_OFFSET_COMPLETE               = 27,
    HCI_EVENT_ID_CONNECTION_PACKET_TYPE_CHANGED           = 28,
    HCI_EVENT_ID_QOS_VIOLATION                            = 29,
    HCI_EVENT_ID_PAGE_SCAN_MODE_CHANGE                    = 30,
    HCI_EVENT_ID_PAGE_SCAN_REPETITION_MODE_CHANGE         = 31,
    HCI_EVENT_ID_FLOW_SPECIFICATION_COMPLETE              = 32,
    HCI_EVENT_ID_INQUIRY_RESULT_WITH_RSSI                 = 33,
    HCI_EVENT_ID_READ_REMOTE_EXTENDED_FEATURES_COMPLETE   = 34,
    HCI_EVENT_ID_SYNCHRONOUS_CONNECTION_COMPLETE          = 43,
    HCI_EVENT_ID_SYNCHRONOUS_CONNECTED_CHANGED            = 44,
    HCI_EVENT_ID_SNIFF_SUBRATING                          = 45,
    HCI_EVENT_ID_EXTENDED_INQUIRY_RESULT                  = 46,
    HCI_EVENT_ID_ENCRYPTION_KEY_REFRESH_COMPLETE          = 47,
    HCI_EVENT_ID_IO_CAPABILITY_REQUEST                    = 48,
    HCI_EVENT_ID_IO_CAPABILITY_REQUEST_REPLY              = 49,
    HCI_EVENT_ID_USER_CONFIRMATION_REQUEST                = 50,
    HCI_EVENT_ID_USER_PASSKEY_REQUEST                     = 51,
    HCI_EVENT_ID_REMOTE_OOB_DATA_REQUEST                  = 52,
    HCI_EVENT_ID_SIMPLE_PAIRING_COMPLETE                  = 53,
    HCI_EVENT_ID_LINK_SUPERVISION_TIMEOUT_CHANGED         = 55,
    HCI_EVENT_ID_ENHANCED_FLUSH_COMPLETE                  = 56,
    HCI_EVENT_ID_USER_PASSKEY_NOTIFICATION                = 58,
    HCI_EVENT_ID_KEYPRESS_NOTIFICATION_EVENT              = 59,
    HCI_EVENT_ID_REMOTE_HOST_SUPPORTED_FEATURES           = 60
} hci_event_id_t;


// These are based on the HCI specification for recommended
// max packet size
#define HCI_MAX_COMMAND_DATA_LENGTH 64
#define HCI_MAX_EVENT_LENGTH        255

typedef struct {
    uint8_t event_code;
    uint8_t parameter_length;
    uint8_t parameters[HCI_MAX_EVENT_LENGTH];  
} hci_event;

typedef struct {
    uint8_t hci_version;
    uint16_t hci_revision;
    uint8_t lmp_version;
    uint16_t manufacture_name;
    uint16_t lmp_subversion;
} hci_local_version_information_t;

#define HCI_IOS_IOCTL_USB_CONTROL   0
#define HCI_IOS_IOCTL_USB_BULK      1
#define HCI_IOS_IOCTL_USB_INTERRUPT 2

#define HCI_ENDPOINT_CONTROL 0x00
#define HCI_ENDPOINT_ACL_OUT 0x02
#define HCI_ENDPOINT_EVENTS  0x81
#define HCI_ENDPOINT_ACL_IN  0x82

#define HCI_EVENT_CODE_INQUIRY_COMPLETE             0x01
#define HCI_EVENT_CODE_INQUIRY_RESULT               0x02
#define HCI_EVENT_CONNECTION_COMPLETE               0x03
#define HCI_EVENT_CODE_REMOTE_NAME_REQUEST_COMPLETE 0x07
#define HCI_EVENT_CODE_COMMAND_COMPLETE             0x0E
#define HCI_EVENT_CODE_COMMAND_STATUS               0x0F
#define HCI_EVENT_HARDWARE_ERROR                    0x10
#define HCI_EVENT_ACL_NUMBER_OF_COMPLETE_PACKETS    0x13

#define HCI_OPCODE_READ_LOCAL_VERSION_INFORMATION 0x1001
#define HCI_OPCODE_READ_LOCAL_SUPPORTED_FEATURES  0x1003
#define HCI_OPCODE_READ_BUFFER_SIZE               0x1005
#define HCI_OPCODE_READ_BD_ADDR                   0x1009
#define HCI_OPCODE_INQUIRY_START                  0x0401
#define HCI_OPCODE_INQUIRY_CANCEL                 0x0402
#define HCI_OPCODE_CREATE_CONNECTION              0x0405
#define HCI_OPCODE_REMOTE_NAME_REQUEST            0x0419
#define HCI_OPCODE_SET_EVENT_MASK                 0x0C01
#define HCI_OPCODE_RESET                          0x0C03

static const char* TAG = "HCI";

typedef struct {
    uint16_t opcode;
    uint8_t error_code;
    uint8_t buffer_size;
    uint8_t reply_size;
    uint8_t* data_buffer;
} hci_command_request;

static struct {
    int file;
    bool task_request_exit;

    TaskHandle_t task;
    SemaphoreHandle_t lock; // Locks usage of HCI
    SemaphoreHandle_t waiter_event; // Reply to HCI command is ready.
    SemaphoreHandle_t outgoing_acl_packets; // Track how many outgoing hcl packets

    // Current HCI Request
    hci_command_request* current_command;
    uint8_t* current_name_request;
    uint8_t* current_connection_request;
    uint8_t name_request_error_code;

    // Settings gathered through initialization
    hci_local_version_information_t lvi;
    uint8_t local_features[8];
    uint8_t bd_address[6];

    uint64_t event_mask;

    // Discovery State
    hci_discovered_device_handler discovery_discover_handler;
    hci_discovery_complete_handler discovery_complete_handler;
    void* discovery_user_data;

    StaticSemaphore_t semaphore_data[4];
} hci_state;

typedef struct {
    uint16_t opcode;
    uint8_t parameter_length;
    uint8_t parameters[HCI_MAX_COMMAND_DATA_LENGTH];
} hci_command_buffer;

hci_buffer_sizes_t hci_buffer_sizes;

static void hci_task(void* unused_1);

/* -------------------HCI Low Level USB--------------------- */

// Sends / Receives a bulk or interrupt request.
// The actual direction depends on the endpoint
// >=0x80 are input endpoints, <0x80 are output
// (wiibrew)
// Data buffer should be 32 byte aligned
static int hci_transfer(uint8_t ioctl, uint8_t endpoint, void* buffer, uint16_t size) {
    alignas(USB_ALIGN) uint8_t b_endpoint_buffer[32];
    alignas(USB_ALIGN) uint16_t w_length_buffer[16]; // Big Endianness This Time

    b_endpoint_buffer[0] = endpoint;
    w_length_buffer[0] = htobe16(size);

    alignas(USB_ALIGN) ios_ioctlv_t vectors[3] = {
        { .data = &b_endpoint_buffer, .size = 1 },
        { .data = &w_length_buffer, .size = 2 },
        { .data = buffer, .size = size },
    };

    int ret = ios_ioctlv(hci_state.file, ioctl, 2, 1, vectors);
    if(ret < 0) {
        HCI_LOG_ERROR(TAG, "Send Command Failed, IOS ioctlv failed: %d", ret);
        return BLERROR_IOS_EXCEPTION;
    }
    return ret;
}

// Send a command off to the HCI.
// params has the parameters to the command, given there are parameters
// And the follow up reply event's data will be stored, and the actull reply size noted.
// NOTE: Its possible to get a reply size bigger than your buffer size, just means you lost some data
static int hci_send_command(uint16_t opcode, const void* params, size_t param_length, uint8_t* reply_buffer, size_t reply_buffer_size, size_t* reply_size) {
    // Create a request for the HCI Task
    // To fill in once it gets the command complete event
    hci_command_request request = {
        .opcode = opcode,
        .error_code = 0,
        .buffer_size = reply_buffer_size,
        .data_buffer = reply_buffer,
        .reply_size = 0
    };
    hci_state.current_command = &request;

    // Assume no reply unless we got one with no error
    if(reply_size != NULL)
        *reply_size = 0;
    
    // Ensure were not about to curropt the stack.
    if(param_length > HCI_MAX_COMMAND_DATA_LENGTH) {
        HCI_LOG_ERROR(TAG, "Command too long. Size %zu, Max: %d", param_length, HCI_MAX_COMMAND_DATA_LENGTH);
        return BLERROR_ARGUMENT;
    }

    // Assemble payload
    alignas(USB_ALIGN) hci_command_buffer payload;
    payload.opcode = htole16(opcode);
    payload.parameter_length = param_length;
    memcpy(payload.parameters, params, param_length);

    // Assemble remaining ioctl vectors
    uint16_t packet_length = sizeof(payload.opcode) + sizeof(payload.parameter_length) + payload.parameter_length;
    alignas(USB_ALIGN) uint8_t bm_request_type = 0x20;
    alignas(USB_ALIGN) uint8_t b_request = 0x00;
    alignas(USB_ALIGN) uint16_t w_value = 0x0;
    alignas(USB_ALIGN) uint16_t w_index = 0x0;
    alignas(USB_ALIGN) uint16_t w_length = htole16(packet_length);
    alignas(USB_ALIGN) uint8_t unknown = 0x0;

    alignas(USB_ALIGN) ios_ioctlv_t vectors[7] = {
        { .data = &bm_request_type, .size = sizeof(bm_request_type) },
        { .data = &b_request, .size = sizeof(b_request) },
        { .data = &w_value, .size = sizeof(w_value) },
        { .data = &w_index, .size = sizeof(w_index) },
        { .data = &w_length, .size = sizeof(w_length) },
        { .data = &unknown, .size = sizeof(unknown) },
        { .data = (void*)&payload, packet_length},
    };

    // Send it through IOS
    int ret = ios_ioctlv(hci_state.file, HCI_IOS_IOCTL_USB_CONTROL, 6, 1, vectors);
    if(ret < 0) {
        HCI_LOG_ERROR(TAG, "Send Command Failed, IOS ioctlv failed: %d", ret);
        return BLERROR_IOS_EXCEPTION;
    }

    // Poll for completion and report error
    xSemaphoreTake(hci_state.waiter_event, portMAX_DELAY);
    if(request.error_code != 0) {
        return BLERROR_HCI_REQUEST_ERR;
    }

    // You got a reply! Good Job
    if(reply_size != NULL)
        *reply_size = request.reply_size;
    
    return ret;
}

// Receives an event.
// Expects a 32 byte aligned event buffer
static int hci_receive_event(hci_event* event) {
    // Transaction
    memset(event, 0xFF, sizeof(*event));
    event->parameter_length = sizeof(event->parameters);
    int ret = hci_transfer(HCI_IOS_IOCTL_USB_INTERRUPT, HCI_ENDPOINT_EVENTS, ((uint8_t*)event), 256);
    if(ret < 0) {
        return ret;
    }

    if(event->parameter_length > sizeof(event->parameters)) {
        HCI_LOG_ERROR(TAG, "Receive event got more bytes than can possibly fit into buffer? That cant be good.");
        return BLERROR_RUNTIME;
    }

    return ret;
}

/* -------------------HCI Internal Commands--------------------- */

// Reads back information about the devices bluetooth version
static int hci_read_local_version_information() {
    uint8_t lvi_buffer[9];
    size_t reply_size;

    int ret;
    ret = hci_send_command(HCI_OPCODE_READ_LOCAL_VERSION_INFORMATION, NULL, 0, lvi_buffer, sizeof(lvi_buffer), &reply_size);
    if(ret < 0) return ret;

    // Make sure we have the right number in the response
    // In accordance with the specification
    if(reply_size < 9) {
       return BLERROR_RUNTIME;
    }

    // Status and Error
    if(lvi_buffer[0] != 0) {
        return BLERROR_RUNTIME;
    }

    // Decode it
    hci_state.lvi.hci_version = lvi_buffer[1];
    hci_state.lvi.hci_revision = ((uint16_t)lvi_buffer[2]) | (lvi_buffer[3] << 8);
    hci_state.lvi.lmp_version = lvi_buffer[4];
    hci_state.lvi.manufacture_name =  ((uint16_t)lvi_buffer[5]) | (lvi_buffer[6] << 8);
    hci_state.lvi.lmp_subversion =  ((uint16_t)lvi_buffer[7]) | (lvi_buffer[8] << 8);

    // Log it for info if we want
    HCI_LOG_INFO(TAG, "Local Version Information:");
    HCI_LOG_INFO(TAG, "  HCI Version:      %02x", hci_state.lvi.hci_version);
    HCI_LOG_INFO(TAG, "  HCI Revision:     %04x", hci_state.lvi.hci_revision);
    HCI_LOG_INFO(TAG, "  LMP Version:      %02x", hci_state.lvi.lmp_version);
    HCI_LOG_INFO(TAG, "  Manufacture Name: %04x", hci_state.lvi.manufacture_name);
    HCI_LOG_INFO(TAG, "  LMP Subversion:   %04x", hci_state.lvi.lmp_subversion);

    return 0;
}

// Reads back local supported features
static int hci_read_local_supported_features() {
    uint8_t buffer[9];
    size_t reply_size;

    int ret;
    ret = hci_send_command(HCI_OPCODE_READ_LOCAL_SUPPORTED_FEATURES, NULL, 0, buffer, sizeof(buffer), &reply_size);
    if(ret < 0) return ret;

    // Make sure we have the right number in the response
    // In accordance with the specification
    if(reply_size < 9) {
       return BLERROR_RUNTIME;
    }

    // Status and Error
    if(buffer[0] != 0) {
        return BLERROR_RUNTIME;
    }

    memcpy(hci_state.local_features, buffer + 1, 8);

    // Log it for info if we want
    HCI_LOG_INFO(TAG, "Local Supported Features:");
    HCI_LOG_INFO(TAG, "  %02X %02X %02X %02X %02X %02X %02X %02X",
        buffer[1], buffer[2], buffer[3], buffer[4],
        buffer[5], buffer[6], buffer[7], buffer[8]);
    
    return 0;
}

// Reads back the bluetooth address of the device
static int hci_read_bd_addr() {
    uint8_t bd_addr_buffer[7];
    size_t reply_size;

    int ret;
    ret = hci_send_command(HCI_OPCODE_READ_BD_ADDR, NULL, 0, bd_addr_buffer, sizeof(bd_addr_buffer), &reply_size);
    if(ret < 0) return ret;

    // Make sure we have the right number in the response
    // In accordance with the specification
    if(reply_size < 7) {
       return BLERROR_RUNTIME;
    }

    // Status and Error
    if(bd_addr_buffer[0] != 0) {
        return BLERROR_RUNTIME;
    }

    memcpy(hci_state.bd_address, bd_addr_buffer+1, sizeof(hci_state.bd_address));

    // Log it for info if we want
    HCI_LOG_INFO(TAG, "BD Address: %02X:%02X:%02X:%02X:%02X:%02X",
        bd_addr_buffer[1], bd_addr_buffer[2], bd_addr_buffer[3], bd_addr_buffer[4], bd_addr_buffer[5], bd_addr_buffer[6]);

    return 0;
}

// Reads back the buffer sizes for various parameters.
static int hci_read_buffer_sizes() {
    uint8_t buffer[8];
    size_t reply_size;

    int ret;
    ret = hci_send_command(HCI_OPCODE_READ_BUFFER_SIZE, NULL, 0, buffer, sizeof(buffer), &reply_size);
    if(ret < 0) return ret;

    // Make sure we have the right number in the response
    // In accordance with the specification
    if(reply_size < 8) {
       return BLERROR_RUNTIME;
    }

    // Status and Error
    if(buffer[0] != 0) {
        return BLERROR_RUNTIME;
    }

    // Decode it
    hci_buffer_sizes.acl_data_packet_length = ((uint16_t)buffer[1]) | (buffer[2] << 8);
    hci_buffer_sizes.synchronous_data_packet_length = buffer[3];
    hci_buffer_sizes.total_num_acl_data_packets = ((uint16_t)buffer[4]) | (buffer[5] << 8);
    hci_buffer_sizes.total_num_synchronous_data_packets = ((uint16_t)buffer[6]) | (buffer[7] << 8);

    // Log it for info if we want
    HCI_LOG_INFO(TAG, "Buffer Sizes:");
    HCI_LOG_INFO(TAG, "  ACL Data Packet Length:            %d", hci_buffer_sizes.acl_data_packet_length);
    HCI_LOG_INFO(TAG, "  Synchonous Data Packet Length:     %d", hci_buffer_sizes.synchronous_data_packet_length);
    HCI_LOG_INFO(TAG, "  Total Num ACL Data Packets:        %d", hci_buffer_sizes.total_num_acl_data_packets);
    HCI_LOG_INFO(TAG, "  Total Num Synchonous Data Packets: %d", hci_buffer_sizes.total_num_synchronous_data_packets);

    return 0;
}

// Sends off the event mask to the hardware
static int hci_set_event_mask() {
    uint8_t buffer[8];

    buffer[0] = hci_state.event_mask >> (0 * 8) & 0xFF;
    buffer[1] = hci_state.event_mask >> (1 * 8) & 0xFF;
    buffer[2] = hci_state.event_mask >> (2 * 8) & 0xFF;
    buffer[3] = hci_state.event_mask >> (3 * 8) & 0xFF;
    buffer[4] = hci_state.event_mask >> (4 * 8) & 0xFF;
    buffer[5] = hci_state.event_mask >> (5 * 8) & 0xFF;
    buffer[6] = hci_state.event_mask >> (6 * 8) & 0xFF;
    buffer[7] = hci_state.event_mask >> (7 * 8) & 0xFF;

    return hci_send_command(HCI_OPCODE_READ_BUFFER_SIZE, buffer, sizeof(buffer), NULL, 0, NULL);
}

// Starts device discovery.
// lap - Inquiry Access Code for what devices to discover.
// length - How long to search. Between 0x01 – 0x30 in units of 1.28 seconds
// responses - How many responses to create. Or 0 for unlimited.
static int hci_start_inquiry_mode(uint32_t lap, uint8_t length, uint8_t responses) {
    uint8_t buffer[5];

    buffer[0] = lap >> (0 * 8) & 0xFF;
    buffer[1] = lap >> (1 * 8) & 0xFF;
    buffer[2] = lap >> (2 * 8) & 0xFF;
    buffer[3] = length;
    buffer[4] = responses;

    return hci_send_command(HCI_OPCODE_INQUIRY_START, buffer, sizeof(buffer), NULL, 0, NULL);
}

// Stops inquiry mode.
static int hci_cancel_inquiry() {
    uint8_t buffer[1];
    size_t reply_size;

    int ret;
    ret = hci_send_command(HCI_OPCODE_INQUIRY_CANCEL, NULL, 0, buffer, sizeof(buffer), NULL);
    if(ret < 0) return ret;

    // Make sure we have the right number in the response
    // In accordance with the specification
    if(reply_size < 8) {
       return BLERROR_RUNTIME;
    }

    // Status and Error
    if(buffer[0] != 0) {
        return BLERROR_RUNTIME;
    }

    return 0;
}

/* -------------------HCI Initialization--------------------- */

int hci_initialize(const char* device) {
    // Known state in case we dont initialize anything
    memset(&hci_state, 0, sizeof(hci_state));

    hci_state.file =  ios_open(device, IOS_MODE_READ | IOS_MODE_WRITE);

    // If there was an issue opening it, report that.
    if(hci_state.file < 0) {
        HCI_LOG_ERROR(TAG, "Error opening \"%s\": %d", device, hci_state.file);
        return hci_state.file;
    }

    // Create semaphores.
    hci_state.lock = xSemaphoreCreateMutexStatic(&hci_state.semaphore_data[0]);
    hci_state.waiter_event = xSemaphoreCreateCountingStatic(5, 0, &hci_state.semaphore_data[1]);

    BaseType_t err = xTaskCreate(hci_task, TAG, HCI_TASK_STACK_SIZE, NULL, HCI_TASK_PRIORITY, &hci_state.task);
    if(err != pdPASS) {
        HCI_LOG_ERROR(TAG, "Failed to create task: %d", err);
        return BLERROR_FREERTOS;
    }

    // Reset HCI
    int ret = hci_reset();
    if(ret < 0)
        return ret;

    //
    // Following a simi-standard initialization sequence.
    //

    // Get Local Version Information
    ret = hci_read_local_version_information();
    if(ret < 0)
        return ret;

    // Read local features
    ret = hci_read_local_supported_features();
    if(ret < 0)
        return ret;

    // Next, We want the BD Address
    ret = hci_read_bd_addr();
    if(ret < 0)
        return ret;
    
    // Next we need buffer sizes
    ret = hci_read_buffer_sizes();
    if(ret < 0)
        return ret;
    
    // Check Buffer Sizes
    if(hci_buffer_sizes.acl_data_packet_length > HCI_MAX_ACL_DATA_LENGTH) {
        HCI_LOG_ERROR(TAG, "ACL packet size too big for statically allocated buffer.");
        return -1;
    }

    // Create ACL max outgoing counter
    hci_state.outgoing_acl_packets = xSemaphoreCreateCountingStatic(hci_buffer_sizes.total_num_acl_data_packets, hci_buffer_sizes.total_num_acl_data_packets, &hci_state.semaphore_data[2]);

    // ACL semaphores
    hcl_acl_packet_out_lock = xSemaphoreCreateMutexStatic(&hci_state.semaphore_data[3]);

    // We have skipped checking device capabilities.
    // I will assume the device is capable of the capabilites needed to connect to wiimotes lol.
    
    // Now we set the event mask
    hci_state.event_mask = (1<<HCI_EVENT_ID_INQUIRY_COMPLETE) | (1<<HCI_EVENT_ID_INQUIRY_RESULT) |
                           (1<<HCI_EVENT_ID_CONNECTION_COMPLETE) |
                           (1<<HCI_EVENT_ID_CONNECTION_REQUEST);
    ret = hci_set_event_mask();
    if(ret < 0) {
        return ret;
    }


    HCI_LOG_INFO(TAG, "Successfully Initialized");
    return 0;
}

void hci_close() {
    // Close Task If Needed
    if(hci_state.task != NULL) {
        hci_state.task_request_exit = true;

        // Will both disconnect all bluetooth devices
        // And then give us the event we needed to get the HCI
        // task to exit.
        hci_reset();

        xSemaphoreTake(hci_state.waiter_event, portMAX_DELAY);

        vTaskDelete(hci_state.task);
    }

    // Close file if needed
    if(hci_state.file > 0) {
        ios_close(hci_state.file);
    }
}

/* -------------------HCI Management Commands--------------------- */

int hci_reset() {
    return hci_send_command(HCI_OPCODE_RESET, NULL, 0, NULL, 0, NULL);
}

int hci_begin_discovery(
        uint32_t lap, uint8_t length, uint8_t responses,
        hci_discovered_device_handler on_discovered, hci_discovery_complete_handler on_complete, void *user_data) {
    // If any handlers are set, thats an error
    if(hci_state.discovery_discover_handler || hci_state.discovery_complete_handler) {
        return BLERROR_RUNTIME;
    }

    // This must at least be set
    // Since it will be cleared when discovery ends
    // Or else, we may override it while discovery is running
    if(on_discovered == NULL)
        return BLERROR_RUNTIME;

    // Set handlers then
    hci_state.discovery_discover_handler = on_discovered;
    hci_state.discovery_complete_handler = on_complete;
    hci_state.discovery_user_data = user_data;

    // Begin Discovery
    int ret;
    ret = hci_start_inquiry_mode(lap, length, responses);
    if(ret < 0) {
        // Clear then back out ;w;
        hci_state.discovery_discover_handler = 0;
        hci_state.discovery_complete_handler = 0;
        return ret;
    }

    return 0;
}

int hci_cancel_discovery() {
    int ret;
    ret = hci_cancel_inquiry();
    if(ret < 0)
        return ret;
    
    // Now we clear them out just in case
    // the event did not
    hci_state.discovery_discover_handler = 0;
    hci_state.discovery_complete_handler = 0;

    return 0;
}

int hci_get_remote_name(const hci_discovered_device_info_t* device, uint8_t* name) {
    uint8_t buffer[10];

    memcpy(buffer, device->address, 6);
    buffer[6] = device->page_scan_repetition_mode;
    buffer[7] = 0x00; // Reserved Value
    buffer[8] = device->clock_offset & 0xFF;
    buffer[9] = device->clock_offset >> 8;

    hci_state.current_name_request = name;

    hci_send_command(HCI_OPCODE_REMOTE_NAME_REQUEST, buffer, sizeof(buffer), NULL, 0, NULL);

    // Poll for completion and report error
    xSemaphoreTake(hci_state.waiter_event, portMAX_DELAY);
    if(hci_state.name_request_error_code != 0) {
        return BLERROR_HCI_REQUEST_ERR;
    }

    return 0;
}

int hci_create_connection(const hci_discovered_device_info_t* device, uint16_t* handle) {
    // Calculate Packet Type (Supported Packets)
    uint16_t packet_type = 0;

    // Mandatory Enables
    packet_type |= 0x0008; // DM1 may be used
    packet_type |= 0x0010; // DH1 may be used

    if (hci_state.local_features[0] & (1<<0)) { // 3 Slot Packets
        packet_type |= 0x0400; // DM3 may be used
        packet_type |= 0x0800; // DH3 may be used
    }
    if (hci_state.local_features[0] & (1<<1)) { // 5 Slot Packets
        packet_type |= 0x4000; // DM5 may be used
        packet_type |= 0x8000; // DH5 may be used
    }

    // EDR bits
    if (!(hci_state.local_features[3] & (1<<1))) // Enhanced Data Rate ACL 2 Mbps mode
        packet_type |= 0x0002 | 0x0100 | 0x1000; // forbid all 2-DHx

    if (!(hci_state.local_features[3] & (1<<2))) // Enhanced Data Rate ACL 3 Mbps mode 
        packet_type |= 0x0004 | 0x0200 | 0x2000; // forbid all 3-DHx

    if (!(hci_state.local_features[4] & (1<<7))) // 3-slot Enhanced Data Rate ACL packets 
        packet_type |= 0x0100 | 0x0200; // forbid 3-slot EDR

    if (!(hci_state.local_features[5] & (1<<0))) // 5-slot Enhanced Data Rate ACL packets 
        packet_type |= 0x1000 | 0x2000; // forbid 5-slot EDR

    
    uint8_t buffer[13];
    uint8_t connection_handle_buffer[11];

    memcpy(buffer, device->address, 6);
    buffer[6] = packet_type & 0xFF;
    buffer[7] = packet_type >> 8;
    buffer[8] = device->page_scan_repetition_mode;
    buffer[9] = 0x0; // Reserved
    buffer[10] = device->clock_offset & 0xFF;
    buffer[11] = device->clock_offset >> 8;
    buffer[12] = 0x0; // No Role Switching

    hci_state.current_connection_request = connection_handle_buffer;

    int ret;
    ret = hci_send_command(HCI_OPCODE_CREATE_CONNECTION, buffer, sizeof(buffer), NULL, 0, NULL);
    if(ret < 0)
        return ret;

    // Wait for the connection to be created
    xSemaphoreTake(hci_state.waiter_event, portMAX_DELAY);

    // Handle Error
    if(connection_handle_buffer[0] != 0) {
        return BLERROR_HCI_CONNECT_FAILED;
    }

    *handle = ((uint16_t)connection_handle_buffer[1] << 0) | ((uint16_t)connection_handle_buffer[2] << 8);

    return 0;
}

/* -------------------HCI ACL/SCL--------------------- */

int hci_send_acl(uint16_t handle, hci_acl_packet_boundary_flag_t pb, hci_acl_packet_broadcast_flag_t bc, uint16_t length) {
    hci_acl_packet_out.handle = htole16((handle & 0x0FFF) | (pb << 12) | (bc << 14));
    hci_acl_packet_out.size = htole16(length);

    // Size not rounded down/up to 32 bytes, is that bad?
    xSemaphoreTake(hci_state.outgoing_acl_packets, portMAX_DELAY);
    return hci_transfer(HCI_IOS_IOCTL_USB_BULK, HCI_ENDPOINT_ACL_OUT, &hci_acl_packet_out, length + 4);
}

int hci_receive_acl(uint16_t *handle, hci_acl_packet_boundary_flag_t *pb, hci_acl_packet_broadcast_flag_t *bc, uint16_t* length) {
    int ret;
    ret = hci_transfer(HCI_IOS_IOCTL_USB_BULK, HCI_ENDPOINT_ACL_IN, &hci_acl_packet_in, sizeof(hci_acl_packet_in));
    if(ret < 0)
        return ret;
    
    uint16_t handle_in = le16toh(hci_acl_packet_in.handle); 
    
    *handle = handle_in & 0x0FFF;
    *pb = (handle_in >> 12) & 3;
    *bc = (handle_in >> 14) & 3;
    *length = le16toh(hci_acl_packet_in.size);

    return 0;
}

/* -------------------HCI Task--------------------- */

static void hci_task_handle_command_complete(const hci_event* event) {
    // Make sure reply is of minimum size
    if(event->parameter_length < 3) {
        HCI_LOG_ERROR(TAG, "Command complete parameters too small.");
        return;
    }

    // Decode Opcode
    uint16_t opcode = ((uint16_t)event->parameters[1]) | (event->parameters[2] << 8);

    // Make sure we have someone waiting on an event
    if(hci_state.current_command == NULL) {
        HCI_LOG_ERROR(TAG, "Supuriuse command complete! Opcode: %04X", opcode);
        return;
    }

    // Make sure our events opcode matches the waiters event opcode
    // Or else who's event is this?
    if(hci_state.current_command->opcode != opcode) {
        HCI_LOG_ERROR(TAG, "Command complete but invalid opcode: %04X. Expected %04X", opcode, hci_state.current_command->opcode);
        return;
    }

    // No Error if we got event complete
    hci_state.current_command->error_code = 0;

    // Copy remaining event parameters to request buffer if provided
    if(hci_state.current_command->data_buffer != NULL) {
        hci_state.current_command->reply_size = event->parameter_length - 3; // account for opcode and command state

        // Truncate size
        size_t copy_size = hci_state.current_command->reply_size > hci_state.current_command->buffer_size ?
                            hci_state.current_command->buffer_size : hci_state.current_command->reply_size;
        
        memcpy(hci_state.current_command->data_buffer, event->parameters + 3, copy_size);
    }

    // Set current request to NULL so we know were done with it and inform any waiters
    hci_state.current_command = NULL;
    xSemaphoreGive(hci_state.waiter_event);
}

static void hci_task_handle_command_status(const hci_event* event) {
    // Make sure it is of minimum size
    if(event->parameter_length < 4) {
        HCI_LOG_ERROR(TAG, "Command status parameters too small.");
        return;
    }

    uint16_t opcode = ((uint16_t)event->parameters[2]) | (event->parameters[3] << 8);

    // Make sure someone is currently waiting for an event
    if(hci_state.current_command == NULL) {
        HCI_LOG_ERROR(TAG, "Supuriuse command status! Opcode: %04X, Status: %02X", opcode, event->parameters[0]);
        return;
    }

    // Make sure the opcode matches the person waiting
    if(hci_state.current_command->opcode != opcode) {
        HCI_LOG_ERROR(TAG, "Command complete but invalid opcode: %04X", opcode);
        return;
    }

    // Copy remaining event parameters to request buffer if provided
    if(hci_state.current_command->data_buffer != NULL) {
        hci_state.current_command->reply_size = event->parameter_length - 3; // account for opcode and command state

        // Truncate size
        size_t copy_size = hci_state.current_command->reply_size > hci_state.current_command->buffer_size ?
                            hci_state.current_command->buffer_size : hci_state.current_command->reply_size;
        
        memcpy(hci_state.current_command->data_buffer, event->parameters + 3, copy_size);
    }

    // Set current request to NULL so we know were done with it and inform any waiters
    hci_state.current_command = NULL;
    xSemaphoreGive(hci_state.waiter_event);
}

static void hci_task_handle_inquiry_complete(const hci_event* event) {
    // Make sure it is of minimum size
    if(event->parameter_length < 1) {
        HCI_LOG_ERROR(TAG, "Inquiry complete parameters too small.");
        return;
    }

    // Make sure status OK
    // Does not return. We trigger handler eather way
    if(event->parameters[0] != 0) {
        HCI_LOG_ERROR(TAG, "Inquiry command failed: %d", event->parameters[0]);
    } else {
        HCI_LOG_INFO(TAG, "Inquiry Command Completed");
    }

    if(hci_state.discovery_complete_handler) {
        hci_state.discovery_complete_handler(hci_state.discovery_user_data, event->parameters[0]);
    }

    // Clear out discovery state since were done
    // And may accept new ones
    hci_state.discovery_complete_handler = NULL;
    hci_state.discovery_discover_handler = NULL;
}

static void hci_task_handle_inquiry_result(const hci_event* event) {
    // Make sure it is of minimum size
    if(event->parameter_length < 1) {
        HCI_LOG_ERROR(TAG, "Inquiry result parameters too small.");
        return;
    }

    int response_count = event->parameters[0];

    // Store info
    hci_discovered_device_info_t info;

    // Step through each discovery
    const uint8_t* buffer = event->parameters + 1;
    size_t remaining = event->parameter_length - 1;
    for(int i = 0; i < response_count; i++) {
        if(remaining < 14) {
            HCI_LOG_ERROR(TAG, "Incomplete inquiry result event.");
            return;
        }

        // Decode it
        memcpy(info.address, buffer, sizeof(info.address));
        info.page_scan_repetition_mode = buffer[6];
        info.class_of_device = ((uint32_t)buffer[9] << 0) | ((uint32_t)buffer[10] << 8) | ((uint32_t)buffer[11] << 16);
        info.clock_offset = ((uint16_t)buffer[12] << 0) | ((uint16_t)buffer[13] << 8);

        HCI_LOG_INFO(TAG, "Discovered %02X:%02X:%02X:%02X:%02X:%02X",
            info.address[0], info.address[1], info.address[2],
            info.address[3], info.address[4], info.address[5]);
        HCI_LOG_INFO(TAG, "  Page Scan Repetition Mode: %02X", info.page_scan_repetition_mode);
        HCI_LOG_INFO(TAG, "  Class Of Device:           %06X", info.class_of_device);
        HCI_LOG_INFO(TAG, "  Clock Offset:              %02X", info.clock_offset);

        // Trigger handler
        if(hci_state.discovery_discover_handler) {
            hci_state.discovery_discover_handler(hci_state.discovery_user_data, &info);
        }

        remaining -= 14;
        buffer += 14;
    }
}

static void hci_task_handle_connection_complete(const hci_event* event) {
    // Make sure it is of minimum size
    if(event->parameter_length < 11) {
        HCI_LOG_ERROR(TAG, "Connection complete parameters too small.");
        return;
    }

    // Make sure we have someone waiting on an event
    if(hci_state.current_connection_request == NULL) {
        HCI_LOG_ERROR(TAG, "Supuriuse connection complete!");
        return;
    }

    hci_state.name_request_error_code = event->parameters[0];
    memcpy(hci_state.current_connection_request, event->parameters, 11);

    // Alert waiter
    hci_state.current_connection_request = NULL;
    xSemaphoreGive(hci_state.waiter_event);
}

static void hci_task_handle_name_request_complete(const hci_event* event) {
    // Make sure it is of minimum size
    if(event->parameter_length != 255) {
        HCI_LOG_ERROR(TAG, "Name request result parameters too small.");
        return;
    }

    // Make sure we have someone waiting on an event
    if(hci_state.current_name_request == NULL) {
        HCI_LOG_ERROR(TAG, "Supuriuse name request complete!");
        return;
    }

    hci_state.name_request_error_code = event->parameters[0];
    memcpy(hci_state.current_name_request, event->parameters + 7, HCI_MAX_NAME_REQUEST_LENGTH);

    // Ensure it ends in zero just in case
    hci_state.current_name_request[HCI_MAX_NAME_REQUEST_LENGTH-1] = 0;

    // Alert waiter
    hci_state.current_name_request = NULL;
    xSemaphoreGive(hci_state.waiter_event);
}

static void hci_task_handle_acl_complete_packets(const hci_event* event) {
    // Make sure it is of minimum size
    if(event->parameter_length < 1) {
        HCI_LOG_ERROR(TAG, "ACL complete packets parameters too small.");
        return;
    }

    int response_count = event->parameters[0];

    const uint8_t* buffer = event->parameters + 1;
    size_t remaining = event->parameter_length - 1;

    for(int i = 0; i < response_count; i++) {
        if(remaining < 4) {
            HCI_LOG_ERROR(TAG, "Complete ACL packets event incomplete.");
            return;
        }

        uint16_t handle = ((uint16_t)buffer[0]) | ((uint16_t)buffer[1] << 16);
        uint16_t packets = ((uint16_t)buffer[2]) | ((uint16_t)buffer[3] << 16);

        // Flush them
        while(packets > 0) {
            xSemaphoreGive(hci_state.outgoing_acl_packets);
            packets--;
        }

        remaining -= 4;
        buffer += 4;
    }
}

SemaphoreHandle_t hcl_acl_packet_out_lock;
hci_event hci_event_buffer MEM2 ALIGN(32);
hci_acl_packet_t hci_acl_packet_out ALIGN(32) MEM2;
hci_acl_packet_t hci_acl_packet_in ALIGN(32) MEM2;

// This task is inchange of managing HCI Events.
static void hci_task(void* unused_1) {
    while(!hci_state.task_request_exit) {
        hci_receive_event(&hci_event_buffer);

        switch(hci_event_buffer.event_code) {
            case HCI_EVENT_CODE_INQUIRY_COMPLETE:
                hci_task_handle_inquiry_complete(&hci_event_buffer);
                break;
            case HCI_EVENT_CODE_INQUIRY_RESULT:
                hci_task_handle_inquiry_result(&hci_event_buffer);
                break;
            case HCI_EVENT_CONNECTION_COMPLETE:
                hci_task_handle_connection_complete(&hci_event_buffer);
                break;
            case HCI_EVENT_CODE_REMOTE_NAME_REQUEST_COMPLETE:
                hci_task_handle_name_request_complete(&hci_event_buffer);
                break;
            case HCI_EVENT_CODE_COMMAND_COMPLETE:
                hci_task_handle_command_complete(&hci_event_buffer);
                break;
            case HCI_EVENT_CODE_COMMAND_STATUS:
                hci_task_handle_command_status(&hci_event_buffer);
                break;
            
            case HCI_EVENT_HARDWARE_ERROR:
                if(hci_event_buffer.parameter_length < 1) {
                    HCI_LOG_ERROR(TAG, "Command hardware error parameters too small.");
                    break;
                }
                HCI_LOG_ERROR(TAG, "Hardware Error Event: %02X", hci_event_buffer.parameters[0]);
                break;
            
            case HCI_EVENT_ACL_NUMBER_OF_COMPLETE_PACKETS:
                hci_task_handle_acl_complete_packets(&hci_event_buffer);
                break;
            
            default:
                HCI_LOG_ERROR(TAG, "Unhandled Event: %02X", hci_event_buffer.event_code);
                break;
        }
    }

    xSemaphoreGive(hci_state.waiter_event);
    
    // Return from task into infinite loop.
    // Because this FreeRTOSs port supports that
}