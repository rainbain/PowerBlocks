/**
 * @file l2cap.c
 * @brief Bluetooth L2CAP Layer
 *
 * L2CAP is a higher level ontop of
 * HCI's ACI packets used to send and receive
 * data from devices.
 * 
 * Its in charge of multiplexing data and
 * reassembling/disassembling packets.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#include "l2cap.h"

#include "hci.h"

#include "powerblocks/core/utils/log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "blerror.h"

#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <stdalign.h>

#define L2CAP_TASK_STACK_SIZE  8192
#define L2CAP_TASK_PRIORITY    (configMAX_PRIORITIES / 4 * 3)

static const char* TAG = "L2CAP";

// How long to wait before a signal times out.
#define L2CAP_SIGNAL_TIMEOUT 1000 // 1s

#define L2CAP_ERROR_LOGGING
//#define L2CAP_INFO_LOGGING
//#define L2CAP_DEBUG_LOGGING

#ifdef L2CAP_ERROR_LOGGING
#define L2CAP_LOG_ERROR(fmt, ...) LOG_ERROR(TAG, fmt, ##__VA_ARGS__)
#else
#define L2CAP_LOG_ERROR(fmt, ...)
#endif 

#ifdef L2CAP_INFO_LOGGING
#define L2CAP_LOG_INFO(fmt, ...) LOG_INFO(TAG, fmt, ##__VA_ARGS__)
#else
#define L2CAP_LOG_INFO(fmt, ...)
#endif 

#ifdef L2CAP_DEBUG_LOGGING
#define L2CAP_LOG_DEBUG(fmt, ...) LOG_DEBUG(TAG, fmt, ##__VA_ARGS__)
#else
#define L2CAP_LOG_DEBUG(fmt, ...)
#endif

static struct {
    TaskHandle_t task;
    SemaphoreHandle_t waiter;
    SemaphoreHandle_t state_lock;

    struct {
        l2cap_device_t** array;
        size_t length;
    } open_handles;

    bool task_running;

    // Signals get an ID so you know their associated response.
    uint8_t current_signal_id;

    StaticSemaphore_t semaphore_data[2];

    // Store a packet we get before being able to handle a connection
    bool floating_packet_available;
    uint16_t floating_packet_handle;
    hci_acl_packet_t floating_packet;

} l2cap_state;

static void l2cap_task(void* unused_1);
static void l2cap_task_buffer_in(const hci_acl_packet_t* packet);
static void l2cap_set_channel_status_flags(l2cap_channel_t* channel, uint16_t flags, uint16_t error);
static int l2cap_send_signal(l2cap_channel_t* channel, l2cap_signal_t* signal);

#define LOCK() xSemaphoreTake(l2cap_state.state_lock, portMAX_DELAY)
#define UNLOCK() xSemaphoreGive(l2cap_state.state_lock)

/* -------------------L2CAP Device & Channel Array Management--------------------- */
static void l2cap_push_device_list(l2cap_device_t* device_handle) {
    LOCK();
    // Attempt to find a open slot before pushing
    for(size_t i = 0; i < l2cap_state.open_handles.length; i++) {
        if(l2cap_state.open_handles.array[i] == NULL) {
            l2cap_state.open_handles.array[i] = device_handle;
            UNLOCK();
            return;
        }
    }

    l2cap_state.open_handles.array = realloc(l2cap_state.open_handles.array, sizeof(l2cap_device_t*) * (l2cap_state.open_handles.length + 1));

    // This is really bad, we just incinerated it.
    ASSERT_OUT_OF_MEMORY(l2cap_state.open_handles.array);

    l2cap_state.open_handles.array[l2cap_state.open_handles.length] = device_handle;
    l2cap_state.open_handles.length++;
    UNLOCK();
}

static void l2cap_remove_device_list(l2cap_device_t* device_handle) {
    // Attempt to find a open slot before pushing
    LOCK();
    for(size_t i = 0; i < l2cap_state.open_handles.length; i++) {
        if(l2cap_state.open_handles.array[i] != device_handle)
            continue;
        
        l2cap_state.open_handles.array[i] = NULL;
        UNLOCK();
        return;
    }
    UNLOCK();
}

static l2cap_device_t* l2cap_find_device_by_hci_handle(uint16_t handle) {
    LOCK();
    for(size_t i = 0; i < l2cap_state.open_handles.length; i++) {
        if(l2cap_state.open_handles.array[i] == NULL)
            continue;

        if(l2cap_state.open_handles.array[i]->handle == handle) {
            UNLOCK();
            return l2cap_state.open_handles.array[i];
        }
    }

    UNLOCK();
    return NULL;
}

static l2cap_device_t* l2cap_find_device_by_mac_address(const uint8_t* mac_address) {
    LOCK();
    for(size_t i = 0; i < l2cap_state.open_handles.length; i++) {
        if(l2cap_state.open_handles.array[i] == NULL)
            continue;
        
        if(memcmp(l2cap_state.open_handles.array[i]->mac_address, mac_address, 6) == 0) {
            UNLOCK();
            return l2cap_state.open_handles.array[i];
        }

    }

    UNLOCK();

    return NULL;
}

static l2cap_channel_t* l2cap_find_channel_by_sid(const l2cap_device_t* device, uint16_t id) {
    LOCK();
    for(size_t i = 0; i < device->open_channels.length; i++) {
        if(device->open_channels.array[i].sid == id) {
            UNLOCK();
            return &device->open_channels.array[i];
        }
    }

    UNLOCK();

    return NULL;
}

static l2cap_channel_t* l2cap_find_channel_by_protocol_id(const l2cap_device_t* device, uint16_t id) {
    LOCK();
    for(size_t i = 0; i < device->open_channels.length; i++) {
        if(device->open_channels.array[i].protocol_id == id) {
            UNLOCK();
            return &device->open_channels.array[i];
        }
    }

    UNLOCK();

    return NULL;
}

/* -------------------L2CAP Signal Channel--------------------- */
#define L2CAP_SIGNAL_CODE_REJECT                 0x01
#define L2CAP_SIGNAL_CODE_CONNECTION_REQUEST     0x02
#define L2CAP_SIGNAL_CODE_CONNECTION_RESPONSE    0x03
#define L2CAP_SIGNAL_CODE_CONFIGURE_REQUEST      0x04
#define L2CAP_SIGNAL_CODE_CONFIGURE_RESPONSE     0x05
#define L2CAP_SIGNAL_CODE_DISCONNECTION_REQUEST  0x06
#define L2CAP_SIGNAL_CODE_DISCONNECTION_RESPONSE 0x07
#define L2CAP_SIGNAL_CODE_ECHO_REQUEST           0x08
#define L2CAP_SIGNAL_CODE_ECHO_RESPONSE          0x09
#define L2CAP_SIGNAL_CODE_INFORMATION_REQUEST    0x0A
#define L2CAP_SIGNAL_CODE_INFORMATION_RESPONSE   0x0B

static void l2cap_on_reject_signal() {
    L2CAP_LOG_ERROR("Signal rejected!");
}

// The device has requested to open a channel. Respond to it
void l2cap_on_connection_request_signal(l2cap_channel_t* signal_channel, const l2cap_signal_t* signal) { 
    if(signal->length < 4) {
        L2CAP_LOG_ERROR("Connection Request Too Small. %d Bytes", signal->length);
        return;
    }

    // Decode it
    uint16_t psm = (uint16_t)signal->data[0] | ((uint16_t)signal->data[1] << 8);
    uint16_t sid = (uint16_t)signal->data[2] | ((uint16_t)signal->data[3] << 8);

    // Begin preparing response
    l2cap_signal_t signal_out;

    // Build response
    signal_out.code = L2CAP_SIGNAL_CODE_CONNECTION_RESPONSE;
    signal_out.id = signal->id;
    signal_out.length = htole16(8);
    
    // SID
    signal_out.data[2] = sid & 0xFF;
    signal_out.data[3] = sid >> 8;

    // Status is ok
    signal_out.data[6] = 0x00;
    signal_out.data[7] = 0x00;

    // Is this a channel the host supports?
    l2cap_channel_t* opened_channel = l2cap_find_channel_by_protocol_id((const l2cap_device_t*)signal_channel->device, psm);

    // No, then you cant open it
    if(opened_channel == NULL) {
        L2CAP_LOG_ERROR("Device attempted to open an unsupported PSM: %04X", psm);
        // Dummy DID, since we dont have one for a nonexistance channel
        signal_out.data[0] = 0x40;
        signal_out.data[1] = 0x00;

        // Result = Connection refused – PSM not supported.
        signal_out.data[4] = 0x02;
        signal_out.data[5] = 0x00;
    } else {
        // The channels sid
        signal_out.data[0] = opened_channel->sid & 0xFF;
        signal_out.data[1] = opened_channel->sid >> 8;

        // Result = Ok
        signal_out.data[4] = 0x00;
        signal_out.data[5] = 0x00;
    }

    // Send response
    int ret = l2cap_send_channel(signal_channel, &signal_out, 12);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Failed to send reply to configuration request on device %04X", ((l2cap_device_t*)signal_channel->device)->handle);
        return;
    }

    // Ok now return if this was not the open channel
    if(opened_channel == NULL)
        return;

    // All open
    opened_channel->did = sid;
    l2cap_set_channel_status_flags(opened_channel, opened_channel->status_flags | L2CAP_CHANNEL_STATUS_OPEN, 0);

    // Send configuration request
    signal_out.code = L2CAP_SIGNAL_CODE_CONFIGURE_REQUEST;
    signal_out.length = 4;

    // Destination + Flags
    signal_out.data[0] = opened_channel->did & 0xFF;
    signal_out.data[1] = opened_channel->did >> 8;
    signal_out.data[2] = 0x00; signal_out.data[3] = 0x00; // Flags

    ret = l2cap_send_signal(signal_channel, &signal_out);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Failed to send configuration request: %d", ret);
        l2cap_set_channel_status_flags(opened_channel, opened_channel->status_flags | L2CAP_CHANNEL_STATUS_ERROR, 0);
    }
}

static void l2cap_on_connection_response_signal(l2cap_channel_t* signal_channel, const l2cap_signal_t* signal) {
    if(signal->length < 8) {
        L2CAP_LOG_ERROR("Connection Response Too Small. %d Bytes", signal->length);
        return;
    }

    // Get the status
    uint16_t status = (uint16_t)signal->data[4] | ((uint16_t)signal->data[5] << 8);

    // Pending status is fine, just wait
    if(status == 0x00001) {
        return;
    }

    // Get channel, the one this signal is for
    uint16_t sid = ((uint16_t)signal->data[2]) | ((uint16_t)signal->data[3] << 8);
    l2cap_channel_t* opened_channel = l2cap_find_channel_by_sid((const l2cap_device_t*)signal_channel->device, sid);

    if(opened_channel == NULL) {
        L2CAP_LOG_ERROR("Got connection response for a channel that does not exist. SID:%04X", sid);
        return;
    }

    // If status is non zero, thats an error
    if(status != 0) {
        l2cap_set_channel_status_flags(opened_channel, opened_channel->status_flags | L2CAP_CHANNEL_STATUS_ERROR, status);
        return;
    }

    // Save the ID the channel has been assigned
    opened_channel->did = ((uint16_t)signal->data[0]) | ((uint16_t)signal->data[1] << 8);

    // Alert of change
    l2cap_set_channel_status_flags(opened_channel, opened_channel->status_flags | L2CAP_CHANNEL_STATUS_OPEN, 0);
}

// Device has let us know about our configuration request
static void l2cap_on_configure_response_signal(l2cap_channel_t* signal_channel, const l2cap_signal_t* signal) {
    if(signal->length < 6) {
        L2CAP_LOG_ERROR("Configuration Response Too Small. %d Bytes", signal->length);
        return;
    }

    // Get the status
    uint16_t status = (uint16_t)signal->data[4] | ((uint16_t)signal->data[5] << 8);

    // Get channel, the one this signal is for
    uint16_t sid = ((uint16_t)signal->data[0]) | ((uint16_t)signal->data[1] << 8);
    l2cap_channel_t* opened_channel = l2cap_find_channel_by_sid((const l2cap_device_t*)signal_channel->device, sid);

    if(opened_channel == NULL) {
        L2CAP_LOG_ERROR("Got configuration response for a channel that does not exist. SID:%04X", sid);
        return;
    }

    // If status is non zero, thats an error
    if(status != 0) {
        l2cap_set_channel_status_flags(opened_channel, opened_channel->status_flags | L2CAP_CHANNEL_STATUS_ERROR, status);
        return;
    }

    // Alert of change
    l2cap_set_channel_status_flags(opened_channel, opened_channel->status_flags | L2CAP_CHANNEL_STATUS_LOCAL_CONFIGURED, 0);
}

// Device has requested to configure us
static void l2cap_on_configure_request_signal(l2cap_channel_t* signal_channel, const l2cap_signal_t* signal) {
    if(signal->length < 4) {
        L2CAP_LOG_ERROR("Configuration Request Too Small. %d Bytes", signal->length);
        return;
    }

    uint16_t did = signal->data[0] | (signal->data[1] << 8);
    uint16_t flags = signal->data[2] | (signal->data[3] << 8);

    L2CAP_LOG_DEBUG("Got Configuration Request:");
    L2CAP_LOG_DEBUG("  Destination CID: %04X", did);
    L2CAP_LOG_DEBUG("  Flags:           %04X", flags);

    // Get the channel being configured. Who needs to be open ofc.
    l2cap_channel_t* configured_channel = l2cap_find_channel_by_sid((const l2cap_device_t*)signal_channel->device, did);
    if(configured_channel == NULL) {
        L2CAP_LOG_ERROR("Got configuration request for a channel that does not exist. did:%04X", did);
        return;
    }

    l2cap_signal_t signal_out;

    // Build response
    signal_out.code = L2CAP_SIGNAL_CODE_CONFIGURE_RESPONSE;
    signal_out.id = signal->id;
    signal_out.length = htole16(10);

    // Header, the provided did
    signal_out.data[0] = configured_channel->did & 0xFF;
    signal_out.data[1] = (configured_channel->did >> 8) & 0xFF;

    // Flags
    signal_out.data[2] = 0x00;
    signal_out.data[3] = 0x00;

    // Result: Success
    signal_out.data[4] = 0x00;
    signal_out.data[5] = 0x00;

    // Options: echo what Wiimote sent
    // Option 1: MTU
    signal_out.data[6] = 0x01;       // Type: MTU
    signal_out.data[7] = 0x02;       // Length: 2
    signal_out.data[8] = 0xB9;       // Value LSB
    signal_out.data[9] = 0x00;       // Value MSB

    // Send response
    int ret = l2cap_send_channel(signal_channel, &signal_out, 14);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Failed to send reply to configuration request on device %04X", ((l2cap_device_t*)signal_channel->device)->handle);

        l2cap_set_channel_status_flags(configured_channel, configured_channel->status_flags | L2CAP_CHANNEL_STATUS_ERROR, 0);
    }

    // Alert of change
    l2cap_set_channel_status_flags(configured_channel, configured_channel->status_flags | L2CAP_CHANNEL_STATUS_REMOTE_CONFIGURED, 0);
}

// Device has request information about us
static void l2cap_on_information_request_signal(l2cap_channel_t* signal_channel, const l2cap_signal_t* signal) {
    if(signal->length < 2) {
        L2CAP_LOG_ERROR("Information Request Too Small. %d Bytes", signal->length);
        return;
    }

    uint16_t info_type = (uint16_t)signal->data[0] | ((uint16_t)signal->data[1] << 8);
    L2CAP_LOG_DEBUG("Got Information Request: %04X", info_type);

    l2cap_signal_t signal_out;

    // Build response
    signal_out.code = L2CAP_SIGNAL_CODE_INFORMATION_RESPONSE;
    signal_out.id = signal->id;
    signal_out.length = htole16(4);

    // Info Type, copied over
    signal_out.data[0] = signal->data[0];
    signal_out.data[1] = signal->data[1];

    // Result = success
    signal_out.data[2] = 0;
    signal_out.data[3] = 0;
    

    // Send response
    int ret = l2cap_send_channel(signal_channel, &signal_out, 8);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Failed to send reply to configuration request on device %04X", ((l2cap_device_t*)signal_channel->device)->handle);
    }
}

// Called from the L2CAP task when we receive a signal
static void l2cap_on_received_signal(void* channel_c, void* param) {
    l2cap_channel_t* channel = (l2cap_channel_t*) channel_c;
    l2cap_signal_t signal;

    int ret = l2cap_receive_channel(channel, &signal, sizeof(signal));
    if(ret < 0 || ret < 4) {
        L2CAP_LOG_ERROR("Failed to receive signal: %d", ret);
        return;
    }

    signal.length = le16toh(signal.length);

    //L2CAP_LOG_INFO("Processing signal %02X", signal.code);
    
    // I just want to say I am sorry for the nesting...
    switch(signal.code) {
        case L2CAP_SIGNAL_CODE_REJECT:
            //l2cap_on_reject_signal();
            printf(".");
            break;
        case L2CAP_SIGNAL_CODE_CONNECTION_REQUEST:
            l2cap_on_connection_request_signal(channel, &signal);
            break;
        case L2CAP_SIGNAL_CODE_CONNECTION_RESPONSE:
            l2cap_on_connection_response_signal(channel, &signal);
            break;
        case L2CAP_SIGNAL_CODE_CONFIGURE_RESPONSE:
            l2cap_on_configure_response_signal(channel, &signal);
            break;
        case L2CAP_SIGNAL_CODE_CONFIGURE_REQUEST:
            l2cap_on_configure_request_signal(channel, &signal);
            break;
        case L2CAP_SIGNAL_CODE_INFORMATION_RESPONSE:
            // Unimplemented, we can just ignore ir
            L2CAP_LOG_DEBUG("Got information response.");
            break;
        case L2CAP_SIGNAL_CODE_INFORMATION_REQUEST: {
            l2cap_on_information_request_signal(channel, &signal);
            break;
        }
        default:
            L2CAP_LOG_ERROR("Did not handle signal %02X!", signal.code);
            break;
    }
}

static int l2cap_send_signal(l2cap_channel_t* channel, l2cap_signal_t* signal) {
    l2cap_device_t* device = (l2cap_device_t*)channel->device;

    int length = signal->length;

    l2cap_state.current_signal_id++;
    uint8_t expected_id = l2cap_state.current_signal_id;

    signal->id = expected_id;
    signal->length = htole16(signal->length);

    int ret;
    ret = l2cap_send_channel(channel, signal, 4 + length);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Failed to send signal on device %04X", ((l2cap_device_t*)channel->device)->handle);
        return ret;
    }
    return 0;
}

static void l2cap_hci_disconnection_complete_handler(void* user_data, uint16_t handle, uint8_t reason) {
    // Lookup the device
    // If not found, thats ok, just means it likely already terminated its l2cap connection.
    l2cap_device_t* device = l2cap_find_device_by_hci_handle(handle);
    if(device == NULL) {
        return;
    }

    L2CAP_LOG_INFO("Device %04X disconnecting. Reason: %d", handle, reason);

    // First close it
    l2cap_close_device(device);

    // Alert handler
    if(device->disconnect_handler) {
        device->disconnect_handler(device->disconnect_handler_data, reason);
    }
}



/* -------------------L2CAP Initialization--------------------- */

int l2cap_initialize() {
    memset(&l2cap_state, 0, sizeof(l2cap_state));

    l2cap_state.waiter = xSemaphoreCreateCountingStatic(5, 0, &l2cap_state.semaphore_data[0]);
    l2cap_state.state_lock = xSemaphoreCreateMutexStatic(&l2cap_state.semaphore_data[1]);

    // Create the wiimote task
    l2cap_state.task_running = true;
    BaseType_t err = xTaskCreate(l2cap_task, TAG, L2CAP_TASK_STACK_SIZE, NULL, L2CAP_TASK_PRIORITY, &l2cap_state.task);
    if(err != pdPASS) {
        L2CAP_LOG_ERROR("Failed to create task: %d", err);
        return BLERROR_FREERTOS;
    }

    // We will handle device disconnect
    hci_set_disconnection_complete_handler(l2cap_hci_disconnection_complete_handler, NULL);

    L2CAP_LOG_INFO("Successfully Initialized");
    return 0;
}

void l2cap_signal_close() {
    l2cap_state.task_running = false;
}

void l2cap_close() {
    xSemaphoreTake(l2cap_state.waiter, portMAX_DELAY);

    vTaskDelete(l2cap_state.task);
}

/* -------------------L2CAP Device & Channel Management--------------------- */

void l2cap_initialize_channel(l2cap_device_t* device, l2cap_channel_t* channel, uint16_t sid, uint16_t did, uint16_t protocol_id, uint8_t* buffer, int buffer_size) {
    // Clear to zeros to be safe
    memset(channel, 0, sizeof(l2cap_channel_t));

    channel->device = device;
    channel->sid = sid;
    channel->did = did; // Set when connected to.
    channel->protocol_id = protocol_id;

    channel->on_complete_packet = xSemaphoreCreateCountingStatic(0xFFFF, 0, &channel->semaphore_data[0]);
    channel->on_status_change = xSemaphoreCreateCountingStatic(0xFFFF, 0, &channel->semaphore_data[1]);
    channel->status_lock = xSemaphoreCreateMutexStatic(&channel->semaphore_data[2]);
    
    channel->buffer = buffer;
    channel->buffer_length = buffer_size;
}

static void l2cap_set_channel_status_flags(l2cap_channel_t* channel, uint16_t flags, uint16_t error) {
    xSemaphoreTake(channel->status_lock, portMAX_DELAY);
    channel->status_flags = flags;
    channel->status_error = error;
    xSemaphoreGive(channel->status_lock);
    
    // Alert anyone waiting on updated status
    xSemaphoreGive(channel->on_status_change);
}

int l2cap_open_device(l2cap_device_t* device_handle, uint16_t hci_device_handle, const uint8_t* mac_address, l2cap_channel_t* channels, size_t channel_count) {
    // Make sure we have not already opened this device
    if(l2cap_find_device_by_mac_address(mac_address) != NULL) {
        L2CAP_LOG_ERROR("Could not open %02X:%02X:%02X:%02X:%02X:%02X, already open.",
        mac_address[0],mac_address[1],mac_address[2],mac_address[3],mac_address[4],mac_address[5]);
        return BLERROR_L2CAP_ALREADY_OPEN;
    }
    
    // Set data struct to zero to be fast and easy
    memset(device_handle, 0, sizeof(*device_handle));

    // Set device handle before we add it. Or else...
    device_handle->lock = xSemaphoreCreateMutexStatic(&device_handle->semaphore_data);
    device_handle->handle = hci_device_handle;

    // We dont add channel 0 the traditional way, because its already open by default
    device_handle->open_channels.array = channels;
    device_handle->open_channels.length = channel_count;

    l2cap_channel_t* sig_channel = l2cap_find_channel_by_sid(device_handle, 0x0001);

    if(sig_channel == NULL) {
        L2CAP_LOG_ERROR("Can not create device, signal channel not provided in channels passed to l2cap_open_device.");
        return BLERROR_ARGUMENT;
    }

    // Configure channel
    l2cap_set_channel_receive_event(sig_channel, l2cap_on_received_signal, NULL);

    // Add this to currently open channels
    l2cap_push_device_list(device_handle);

    // Handle any floating packets
    LOCK();
    if(l2cap_state.floating_packet_available && l2cap_state.floating_packet_handle == hci_device_handle) {
        UNLOCK();
        l2cap_task_buffer_in(&l2cap_state.floating_packet);
        LOCK();
        l2cap_state.floating_packet_available = false;
    }
    UNLOCK();

    return 0;
}

void l2cap_close_device(l2cap_device_t* device_handle) {
    // Remove device from listing
    l2cap_remove_device_list(device_handle);
}

void l2cap_set_disconnect_handler(l2cap_device_t* device_handle, l2cap_disconnect_handler_t handler, void* user_data) {
    // Lock the device while setting it
    xSemaphoreTake(device_handle->lock, portMAX_DELAY);
    device_handle->disconnect_handler = handler;
    device_handle->disconnect_handler_data = user_data;
    xSemaphoreGive(device_handle->lock);
}

// Request to configure a channel
// But this must be sent for wiimotes to actually send reports.
static int l2cap_channel_configuration_request(l2cap_channel_t* channel) {
    l2cap_device_t* device_handle = (l2cap_device_t*)channel->device;

    // Get signal channel
    l2cap_channel_t* signal_channel = l2cap_find_channel_by_sid(device_handle, 0x0001);
    if(signal_channel == NULL) {
        L2CAP_LOG_ERROR("Failed to find signal channel.");
        return BLERROR_RUNTIME;
    }

    // Mark the channel as unconfigured if it was already
    l2cap_set_channel_status_flags(channel, channel->status_flags & ~L2CAP_CHANNEL_STATUS_LOCAL_CONFIGURED, 0);

    // Send configuration request
    l2cap_signal_t sig;
    sig.code = L2CAP_SIGNAL_CODE_CONFIGURE_REQUEST;
    sig.length = 4;

    // Destination + Flags
    sig.data[0] = channel->did & 0xFF;
    sig.data[1] = channel->did >> 8;
    sig.data[2] = 0x00; sig.data[3] = 0x00; // Flags

    int ret = l2cap_send_signal(signal_channel, &sig);
    if(ret < 0)
        return ret;
    
    // Wait for channel to become configured
    uint16_t status;
    ret = l2cap_wait_channel_status(channel, L2CAP_CHANNEL_STATUS_LOCAL_CONFIGURED, &status);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Configuration Request Rejected: %04X", status);
        return ret;
    }

    return 0;
}

int l2cap_open_channel(l2cap_device_t* device_handle, l2cap_channel_t* channel) {
    l2cap_signal_t sig;
    sig.code = L2CAP_SIGNAL_CODE_CONNECTION_REQUEST;
    sig.length = 4;
    sig.data[0] = channel->protocol_id & 0xFF;
    sig.data[1] = channel->protocol_id >> 8;
    sig.data[2] = channel->sid & 0xFF;
    sig.data[3] = channel->sid >> 8;

    l2cap_channel_t* signal_channel = l2cap_find_channel_by_sid(device_handle, 0x0001);
    if(signal_channel == NULL) {
        L2CAP_LOG_ERROR("Failed to find signal channel.");
        return BLERROR_RUNTIME;
    }

    int ret = l2cap_send_signal(signal_channel, &sig);
    if(ret < 0)
        return ret;

    // Wait for channel to become open
    uint16_t status;
    ret = l2cap_wait_channel_status(channel, L2CAP_CHANNEL_STATUS_OPEN, &status);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Configuration Request Rejected: %04X", status);
        return ret;
    }

    // You will need to configure the channel too for it to work
    ret = l2cap_channel_configuration_request(channel);
    return ret;
}

/* -------------------L2CAP Send & Receive Functions--------------------- */

int l2cap_send_channel(l2cap_channel_t* channel, const void* data, uint16_t size) {
    l2cap_device_t* device_handle = (l2cap_device_t*)channel->device;
    
    const uint8_t* data_current = (uint8_t*)data;
    const uint8_t* data_end = data_current + size;

    // We need to lock ACL till were done
    xSemaphoreTake(hcl_acl_packet_out_lock, portMAX_DELAY);

    // Write data buffer
    hci_acl_packet_out.data[0] = size & 0xFF;
    hci_acl_packet_out.data[1] = size >> 8;
    hci_acl_packet_out.data[2] = channel->did & 0xFF;
    hci_acl_packet_out.data[3] = channel->did >> 8;

    L2CAP_LOG_DEBUG("Sending on channel %04X", channel->did);

    // Write initial payload
    size_t remaining = data_end - data_current;
    size_t tx_size = (remaining > hci_buffer_sizes.acl_data_packet_length - 4) ? hci_buffer_sizes.acl_data_packet_length - 4 : remaining;
    memcpy(hci_acl_packet_out.data + 4, data_current, tx_size);

    // Lock, we dont want multiple people writing at the same time
    xSemaphoreTake(device_handle->lock, portMAX_DELAY);

    // Send it
    int ret;
    ret = hci_send_acl(device_handle->handle, HCI_ACL_PACKET_BOUNDARY_FLAG_FIRST_AUTOMATICALLY_FLUSHABLE_PACKET, HCI_ACL_PACKET_BROADCAST_FLAG_PTP,
        tx_size + 4);

    data_current += tx_size;

    if(data_current >= data_end || ret < 0) {
        xSemaphoreGive(device_handle->lock);
        xSemaphoreGive(hcl_acl_packet_out_lock);
        return ret;
    }
    
    // Keep going
    while(data_current < data_end && ret >= 0) {
        // Write payload
        remaining = data_end - data_current;
        tx_size = (remaining > hci_buffer_sizes.acl_data_packet_length) ? hci_buffer_sizes.acl_data_packet_length  : remaining;
        memcpy(hci_acl_packet_out.data, data_current, tx_size);

        ret = hci_send_acl(device_handle->handle, HCI_ACL_PACKET_BOUNDARY_FLAG_CONTINUING_FRAGMENT, HCI_ACL_PACKET_BROADCAST_FLAG_PTP,
            tx_size);
        
        data_current += tx_size;
    }

    xSemaphoreGive(device_handle->lock);
    xSemaphoreGive(hcl_acl_packet_out_lock);
    return ret;
}

int l2cap_receive_channel(l2cap_channel_t* channel, void* data, uint16_t size) {
    l2cap_device_t* device_handle = (l2cap_device_t*) channel->device;

    // Wait for a packet to come in, or pick one up if already there
    xSemaphoreTake(channel->on_complete_packet, portMAX_DELAY);

    // Lock handle
    xSemaphoreTake(device_handle->lock, portMAX_DELAY);

    int mask = channel->buffer_length - 1;

    // Calculate available bytes
    int fifo_available = (channel->fifo_write_head_packet - channel->fifo_read_head ) & mask;

    // Thats ok I our buffer must of overflow.
    // Error reported earlier
    if(fifo_available == 0){
        xSemaphoreGive(device_handle->lock);
    }
    
    // Ok thats weird, there should at least be a size?
    if(fifo_available < 2) {
        L2CAP_LOG_ERROR("Got a less than 2 byte packet in channel FIFO. Channel %04X, Device %04X", channel->sid, device_handle->handle);
        
        // Skip over data.
        channel->fifo_read_head = (channel->fifo_read_head + fifo_available) & mask;
        
        xSemaphoreGive(device_handle->lock);
        return BLERROR_RUNTIME;
    }

    // Read back that size
    int packet_size = ((int)channel->buffer[channel->fifo_read_head] << 8) | (int)channel->buffer[(channel->fifo_read_head + 1) & mask];
    
    // Iterate available and read head after reading 2 bytes.
    fifo_available -= 2;
    channel->fifo_read_head = (channel->fifo_read_head + 2) & mask;

    // If the output buffer is smaller, thats ok, we will just return the full size so they can catch that.
    int copy_size = size > packet_size ? packet_size : size;

    // Ok thats also weird, shouldn't we have only complete packets
    if(packet_size > fifo_available) {
        L2CAP_LOG_ERROR("Got incomplete packet in channel FIFO. Channel %04X, Device %04X", channel->sid, device_handle->handle);
        
        // Skip over data. In this case, just go over everything.
        channel->fifo_read_head = channel->fifo_write_head_packet;
        
        xSemaphoreGive(device_handle->lock);
        return BLERROR_RUNTIME;
    }

    // Copy it.
    int end = channel->fifo_read_head + copy_size;
    if(end > channel->buffer_length) { // Wrap around case
        memcpy(data,
            channel->buffer + channel->fifo_read_head,
            channel->buffer_length - channel->fifo_read_head);
        memcpy((uint8_t*)data + (channel->buffer_length - channel->fifo_read_head), 
            channel->buffer,
            end & mask);
    } else { // No wrap case.
        memcpy(data, channel->buffer + channel->fifo_read_head, copy_size);
    }

    // Iterate read head
    channel->fifo_read_head = (channel->fifo_read_head + packet_size) & mask;

    // Were done here
    xSemaphoreGive(device_handle->lock);

    return packet_size;
}

void l2cap_set_channel_receive_event(l2cap_channel_t* channel, l2cap_channel_event_t event, void* param) {
    channel->event_packet_available.data = param;
    channel->event_packet_available.event = event;
}


int l2cap_wait_channel_status(l2cap_channel_t* channel, uint16_t flags, uint16_t *error_code) {
    while(true) {
        xSemaphoreTake(channel->status_lock, portMAX_DELAY);

        // Error
        if(channel->status_flags & L2CAP_CHANNEL_STATUS_ERROR) {
            if(error_code != NULL)
                *error_code = channel->status_error;
            xSemaphoreGive(channel->status_lock);
            return BLERROR_L2CAP_SIGNAL_FAILED;
        }

        if((channel->status_flags & flags) == flags) {
            if(error_code != NULL)
                *error_code = channel->status_error;
            xSemaphoreGive(channel->status_lock);
            return 0;
        }
        xSemaphoreGive(channel->status_lock);

        // Wait for a change
        if(xSemaphoreTake(channel->on_status_change, L2CAP_SIGNAL_TIMEOUT / portTICK_PERIOD_MS) != pdTRUE) {
            // Timeout
            return BLERROR_TIMEOUT;
        }
    }
}

/* -------------------L2CAP Task--------------------- */

// Function to just write out data to a channels internal buffer
void l2cap_task_write_channel_buffer(l2cap_channel_t* channel, const uint8_t* buffer, size_t size) {  
    int mask = channel->buffer_length - 1;
    
    // Calculate space used in fifo. Extra addition to ensure positive dividend.
    int fifo_used = (channel->fifo_write_head - channel->fifo_read_head ) & mask;
    
    // Remaining size in the fifo, to get the max copy size without overflowing
    int fifo_remaining = channel->buffer_length - 1 - fifo_used;
    bool truncating = size > fifo_remaining;
    int copy_size = truncating ? fifo_remaining : size;

    if(truncating) {
        L2CAP_LOG_INFO("Channel storage buffer overflowed for channel %04X, handle %04X", channel->sid, ((l2cap_device_t*)channel->device)->handle);
    }

    // Write it out
    int end = channel->fifo_write_head + copy_size;
    if (end > channel->buffer_length) {
        memcpy(channel->buffer + channel->fifo_write_head,
            buffer,
            channel->buffer_length - channel->fifo_write_head);
        memcpy(channel->buffer,
            buffer + (channel->buffer_length - channel->fifo_write_head),
            end & mask);
    } else { // Normal all forwards case
        memcpy(channel->buffer + channel->fifo_write_head, buffer, copy_size);
    }

    // Update write head
    channel->fifo_write_head = (channel->fifo_write_head + copy_size) & mask;
}

// Write out the start of packet to a channel
// This just, skips the 2 bytes, for the packet size, so that we reserve them for work
static void l2cap_task_write_channel_start_of_packet(l2cap_channel_t* channel) {
    int mask = channel->buffer_length - 1;

    // Calculate space used in fifo. Extra addition to ensure positive dividend.
    int fifo_used = (channel->fifo_write_head - channel->fifo_read_head) & mask;
    
    // Remaining size in the fifo, to get the max copy size without overflowing
    int fifo_remaining = channel->buffer_length - 1 - fifo_used;

    if(fifo_remaining < 2)
        return;
    
    // Update write head
    channel->fifo_write_head = (channel->fifo_write_head + 2) & mask;
}

// Writes out when a packet is complete.
static void l2cap_task_write_channel_end_of_packet(l2cap_channel_t* channel) {
    int mask = channel->buffer_length - 1;

    // Calculate space used in fifo. Extra addition to ensure positive dividend.
    int fifo_used = (channel->fifo_write_head - channel->fifo_read_head) & mask;
    
    // Remaining size in the fifo, to get the max copy size without overflowing
    int fifo_remaining = channel->buffer_length - 1 - fifo_used;

    if(fifo_remaining < 2)
        goto END;
    
    int packet_size = ((channel->fifo_write_head - channel->fifo_write_head_packet) & mask) - 2;

    // In case we failed to reserve out the 2 bytes for the size, ignore this.
    if(packet_size < 0)
        goto END;

    // Write Packet Size
    /// TODO: Is that first AND needed?
    channel->buffer[channel->fifo_write_head_packet & mask]     = (packet_size >> 8) & 0xFF;
    channel->buffer[(channel->fifo_write_head_packet + 1) & mask] = (packet_size >> 0) & 0xFF;
    
    // Update the last packet
    channel->fifo_write_head_packet = channel->fifo_write_head;

END:
    // Alert reader of new packet
    xSemaphoreGive(channel->on_complete_packet);
}

// Updates the remaining bytes
static void l2cap_task_update_remaining(l2cap_device_t* device, size_t bytes_in) {
    if(bytes_in > device->reading_remaining) {
        L2CAP_LOG_ERROR("More bytes in than remaining on device %04X", device->handle);
        device->reading_remaining = 0;
        return;
    }

    device->reading_remaining -= bytes_in;
}

static void l2cap_task_buffer_in(const hci_acl_packet_t* packet) {
    uint16_t handle_in;
    hci_acl_packet_boundary_flag_t pb_in;
    hci_acl_packet_broadcast_flag_t bc_in;
    uint16_t length_in;
    hci_decode_received_acl(&handle_in, &pb_in, &bc_in, &length_in, packet);

    // Make sure we know how to process it
    if((pb_in != HCI_ACL_PACKET_BOUNDARY_FLAG_FIRST_AUTOMATICALLY_FLUSHABLE_PACKET && pb_in != HCI_ACL_PACKET_BOUNDARY_FLAG_CONTINUING_FRAGMENT) ||
        bc_in != HCI_ACL_PACKET_BROADCAST_FLAG_PTP) {
            
        L2CAP_LOG_ERROR("L2CAP is not currently programmed to handle PB: %d, BC: %d", pb_in, bc_in);
        return;
    }

    /// TODO: Do you really need to look this up EVERY time
    // Find out handle
    l2cap_device_t* device = l2cap_find_device_by_hci_handle(handle_in);
    // Handle packets that come in before the handle is opened.
    if(device == NULL) {
        LOCK();
        if(l2cap_state.floating_packet_available) {
            L2CAP_LOG_ERROR("Got floating packet, but we already have one!");
        }else {
            memcpy(&l2cap_state.floating_packet, packet, sizeof(hci_acl_packet_t));
            l2cap_state.floating_packet_handle = handle_in;
            l2cap_state.floating_packet_available = true;
        }
        UNLOCK();
        return;
    }

    // Ok lock it since were using this device now
    xSemaphoreTake(device->lock, portMAX_DELAY);

    bool channel_on_receive_event_queued = false;

    // Start of new packet, or not
    l2cap_channel_t* channel;
    if(pb_in == HCI_ACL_PACKET_BOUNDARY_FLAG_FIRST_AUTOMATICALLY_FLUSHABLE_PACKET) {
        if(device->reading_remaining != 0) {
            L2CAP_LOG_ERROR("%d bytes left over on handle %04X. Data loss?", device->reading_remaining, handle_in);
        }

        // Make sure theres actually data
        if(length_in < 4) {
            L2CAP_LOG_ERROR("Not enough bytes for start of packet. Handle %04X", device->handle);
            xSemaphoreGive(device->lock);
            return;
        }

        uint16_t packet_size = ((uint16_t)packet->data[0]) | ((uint16_t)packet->data[1] << 8);
        uint16_t packet_channel = ((uint16_t)packet->data[2]) | ((uint16_t)packet->data[3] << 8);

        // Get new channel
        channel = l2cap_find_channel_by_sid(device, packet_channel);

        // Device Read State
        device->reading_channel = channel;
        device->reading_remaining = packet_size;

        if(channel == NULL) {
            L2CAP_LOG_ERROR("Got packet for a channel thats not open, Handle %04X, Channel: %04X", handle_in, packet_channel);
        } else {
            // Begin new packet
            l2cap_task_write_channel_start_of_packet(channel);

            l2cap_task_write_channel_buffer(channel, packet->data + 4, length_in - 4);
            l2cap_task_update_remaining(device, length_in - 4);
        }

        // Alert thread if this was the whole packet
        if(device->reading_remaining == 0) {
            if(channel)l2cap_task_write_channel_end_of_packet(channel);
            channel_on_receive_event_queued = true;
        }
    } else {
        // It needs to be open
        if(device->reading_channel == NULL || device->reading_channel->buffer == NULL) {
            xSemaphoreGive(device->lock);
            return;
        }

        // Write data
        l2cap_task_write_channel_buffer(device->reading_channel, packet->data, length_in);
        l2cap_task_update_remaining(device, length_in);

        // Alert thread if this was the whole packet
        if(device->reading_remaining == 0) {
            l2cap_task_write_channel_end_of_packet(device->reading_channel);
            channel_on_receive_event_queued = true;
        }
    }

    xSemaphoreGive(device->lock);

    if(channel_on_receive_event_queued && channel->event_packet_available.event) {
        channel->event_packet_available.event(channel, channel->event_packet_available.data);
    }
}

static void l2cap_acl_in_handle_0(void* params, int return_value) {
    QueueHandle_t queue = (QueueHandle_t) params;

    uint8_t item = 0;
    if(return_value < 0) { // Report Error
        item = 255;
    }

    xQueueSendFromISR(queue, &item, &exception_isr_context_switch_needed);
}


static void l2cap_acl_in_handle_1(void* params, int return_value) {
    QueueHandle_t queue = (QueueHandle_t) params;

    uint8_t item = 1;
    if(return_value < 0) { // Report Error
        item = 255;
    }

    xQueueSendFromISR(queue, &item, &exception_isr_context_switch_needed);
}

void l2cap_task(void* unused_1) {
    // Double buffering, so we need two buffers for calling
    alignas(32) uint8_t buffer_0_ipc[HCI_ACL_RECEIVE_IPC_BUFFER_SIZE];
    alignas(32) uint8_t buffer_1_ipc[HCI_ACL_RECEIVE_IPC_BUFFER_SIZE];

    // Queue of buffer updates
    StaticQueue_t static_queue;
    uint8_t queue_storage[2];

    QueueHandle_t queue_handle = xQueueCreateStatic(
        sizeof(queue_storage) / sizeof(queue_storage[0]),
        sizeof(queue_storage[0]),
        queue_storage, &static_queue);

    if(queue_handle == NULL) {
        L2CAP_LOG_ERROR("Failed to create ACL queue.");
        return;
    }

    hci_receive_acl_async(&hci_acl_packet_in_0, buffer_0_ipc, l2cap_acl_in_handle_0, queue_handle);
    hci_receive_acl_async(&hci_acl_packet_in_1, buffer_1_ipc, l2cap_acl_in_handle_1, queue_handle);

    uint8_t item;
    while(l2cap_state.task_running) {
        // Get item from queue
        if(xQueueReceive(queue_handle, &item, portMAX_DELAY) != pdPASS) {
            continue;
        }

        // If error, do that
        if(item == 255) {
            L2CAP_LOG_ERROR("ACL Input Error.");
            continue;
        }

        // Process and requeue
        if(item == 0) {
            l2cap_task_buffer_in(&hci_acl_packet_in_0);
            hci_receive_acl_async(&hci_acl_packet_in_0, buffer_0_ipc, l2cap_acl_in_handle_0, queue_handle);
        } else if(item == 1) {
            l2cap_task_buffer_in(&hci_acl_packet_in_1);
            hci_receive_acl_async(&hci_acl_packet_in_1, buffer_1_ipc, l2cap_acl_in_handle_1, queue_handle);
        }
    }

    xSemaphoreGive(l2cap_state.waiter);
    L2CAP_LOG_INFO("Task Exiting...");
    // Return from task into infinite loop.
    // Because this FreeRTOSs port supports that
}