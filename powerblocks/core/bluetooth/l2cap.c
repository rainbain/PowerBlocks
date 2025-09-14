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

#define L2CAP_TASK_STACK_SIZE  8192
#define L2CAP_TASK_PRIORITY    (configMAX_PRIORITIES / 4 * 3)

static const char* TAG = "L2CAP";

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

    struct {
        l2cap_device_t** array;
        size_t length;
    } open_handles;

    bool task_running;

    // Signals get an ID so you know their associated response.
    uint8_t current_signal_id;

    StaticSemaphore_t semaphore_data[1];
} l2cap_state;

static void l2cap_task(void* unused_1);

/* -------------------L2CAP Device & Channel Array Management--------------------- */

static void l2cap_push_device_list(l2cap_device_t* device_handle) {
    // Attempt to find a open slot before pushing
    for(size_t i = 0; i < l2cap_state.open_handles.length; i++) {
        if(l2cap_state.open_handles.array[i] == NULL) {
            l2cap_state.open_handles.array[i] = device_handle;
            return;
        }
    }

    l2cap_state.open_handles.array = realloc(l2cap_state.open_handles.array, sizeof(l2cap_device_t*) * (l2cap_state.open_handles.length + 1));

    // This is really bad, we just incinerated it.
    ASSERT_OUT_OF_MEMORY(l2cap_state.open_handles.array);

    l2cap_state.open_handles.array[l2cap_state.open_handles.length] = device_handle;
    l2cap_state.open_handles.length++;
}

static void l2cap_push_channel(l2cap_channel_t* channel) {
    l2cap_device_t* device = (l2cap_device_t*)channel->device;

    // Attempt to find a open slot before pushing
    for(size_t i = 0; i < device->open_channels.length; i++) {
        if(device->open_channels.array[i] == NULL) {
            device->open_channels.array[i] = channel;
            return;
        }
    }

    device->open_channels.array = realloc(device->open_channels.array, sizeof(l2cap_channel_t*) * (device->open_channels.length + 1));

    // This is really bad, we just incinerated it.
    ASSERT_OUT_OF_MEMORY(device->open_channels.array);

    device->open_channels.array[device->open_channels.length] = channel;
    device->open_channels.length++;
}

static l2cap_device_t* l2cap_find_device_by_hci_handle(uint16_t handle) {
    for(size_t i = 0; i < l2cap_state.open_handles.length; i++) {
        if(l2cap_state.open_handles.array[i] == NULL)
            continue;

        if(l2cap_state.open_handles.array[i]->handle == handle) {
            return l2cap_state.open_handles.array[i];
        }
    }

    return NULL;
}

static l2cap_channel_t* l2cap_find_channel_by_sid(const l2cap_device_t* device, uint16_t id) {
    for(size_t i = 0; i < device->open_channels.length; i++) {
        if(device->open_channels.array[i] == NULL)
            continue;
        
        if(device->open_channels.array[i]->sid == id) {
            return device->open_channels.array[i];
        }
    }

    return NULL;
}

// Find the next free SID (Source CID) for a new L2CAP channel
static uint16_t l2cap_next_free_scid(const l2cap_device_t* device) {
    uint16_t sid = 0x0040;

    while (sid != 0x0000) {
        if (l2cap_find_channel_by_sid(device, sid) == NULL) {
            return sid; // free slot
        }
        sid++;
    }

    // No free CIDs found (shouldn’t really happen unless we have more channels than really possible)
    return 0;
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
    
    // If true the request gets passed on and not dealt with here.
    bool pass_on_request = false;
    bool send_error = false;
    
    switch(signal.code) {
        case L2CAP_SIGNAL_CODE_REJECT:
            pass_on_request = true; // Pass on rejections
            break;
        case L2CAP_SIGNAL_CODE_CONNECTION_RESPONSE:
            // If its pending we not pending pass it on
            if(signal.length < 8) {
                L2CAP_LOG_ERROR("Connection Response Too Small. %d Bytes", signal.length);
                send_error = true;
            } else {
                uint16_t status = (uint16_t)signal.data[6] | ((uint16_t)signal.data[7] << 8);
                L2CAP_LOG_DEBUG("Got Connection Status: %04X", status);
                if(status != 0x0001) { // Not Pending
                    pass_on_request = true;
                }
            }
            break;
        case L2CAP_SIGNAL_CODE_CONFIGURE_RESPONSE:
            if(signal.length < 8) {
                L2CAP_LOG_ERROR("Configuration Response Too Small. %d Bytes", signal.length);
                send_error = true;
            } else {
                pass_on_request = true;
            }
            break;
        case L2CAP_SIGNAL_CODE_CONFIGURE_REQUEST:
            if(signal.length < 4) {
                L2CAP_LOG_ERROR("Configuration Request Too Small. %d Bytes", signal.length);
                send_error = true;
                break;
            }
            L2CAP_LOG_DEBUG("Got Configuration Request:");
            L2CAP_LOG_DEBUG("  Destination CID: %02X%02X", signal.data[1], signal.data[0]);
            L2CAP_LOG_DEBUG("  Flags:           %02X%02X", signal.data[3], signal.data[2]);

            // Send response
            signal.code = L2CAP_SIGNAL_CODE_CONFIGURE_RESPONSE;
            // Id stays the same.
            signal.length = 6;
            // First 2 payload bytes stay the same, Source CID
            signal.data[2] = 0x00; signal.data[3] = 0x00; // Flags
            signal.data[4] = 0x00; signal.data[5] = 0x00; // Result
            break;
        default:
            L2CAP_LOG_ERROR("Did not handle signal %02X!", signal.code);

            // Send response
            send_error = true;
            break;
    }

    if(pass_on_request) {
        // If this is for a task, send it there
        if(channel->incoming_signal != NULL && channel->incoming_id == signal.id) {
            memcpy(channel->incoming_signal, &signal, sizeof(signal));
            xSemaphoreGive(channel->signal_waiter);
            channel->incoming_signal = NULL;
            return;
        } else {
            // Ok so who was this for then?
            send_error = true;
            L2CAP_LOG_ERROR("Got a %02X signal with no waiter!", signal.code);
        }
    }

    if(send_error) {
        L2CAP_LOG_DEBUG("Sending error packet.");
        // Send response
        signal.code = L2CAP_SIGNAL_CODE_REJECT;
        // Id stays the same.
        signal.length = 2;
        signal.data[0] = 0x00; signal.data[1] = 0x00; // Reason = Command Not Understood
    }

    int length = signal.length;
    signal.length = htole16(signal.length);
    ret = l2cap_send_channel(channel, &signal, 4 + length);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Failed to send reply signal on device %04X", ((l2cap_device_t*)channel->device)->handle);
    }
}

// Sends a request, reads back into the signal_io
// Will report rejected request, and ones of invalid response
static int l2cap_send_signal(l2cap_channel_t* channel, l2cap_signal_t* signal, uint8_t expected_response) {
    l2cap_device_t* device = (l2cap_device_t*)channel->device;

    int length = signal->length;

    l2cap_state.current_signal_id++;
    uint8_t expected_id = l2cap_state.current_signal_id;

    signal->id = expected_id;
    signal->length = htole16(signal->length);

    // Set what we want to wait on
    channel->incoming_id = expected_id;
    channel->incoming_signal = signal;

    int ret;
    ret = l2cap_send_channel(channel, signal, 4 + length);
    if(ret < 0) {
        L2CAP_LOG_ERROR("Failed to send signal on device %04X", ((l2cap_device_t*)channel->device)->handle);
        return ret;
    }

    // Wait for our signal
    xSemaphoreTake(channel->signal_waiter, portMAX_DELAY);

    // Oh deer did we get rejected?
    if(signal->code == L2CAP_SIGNAL_CODE_REJECT) {
        uint16_t reason = ((uint16_t)signal->data[0]) | ((uint16_t)signal->data[0] << 8);
        L2CAP_LOG_ERROR("Signal rejected, reason %04X", reason);
        return BLERROR_RUNTIME;
    }

    // Who is this signal for again?
    if(signal->code != expected_response) {
        L2CAP_LOG_ERROR("Got signal of unexpected response. Expected %02X, got %02X", expected_response, signal->code);
        return BLERROR_RUNTIME;
    }

    return 0;
}



/* -------------------L2CAP Initialization--------------------- */

int l2cap_initialize() {
    memset(&l2cap_state, 0, sizeof(l2cap_state));

    l2cap_state.waiter = xSemaphoreCreateCountingStatic(5, 0, &l2cap_state.semaphore_data[0]);

    // Create the wiimote task
    l2cap_state.task_running = true;
    BaseType_t err = xTaskCreate(l2cap_task, TAG, L2CAP_TASK_STACK_SIZE, NULL, L2CAP_TASK_PRIORITY, &l2cap_state.task);
    if(err != pdPASS) {
        L2CAP_LOG_ERROR("Failed to create task: %d", err);
        return BLERROR_FREERTOS;
    }


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

// Since multiple parts of L2CAP create the channel data structure
// This is provided
static void l2cap_initialize_channel(l2cap_device_t* device, l2cap_channel_t* channel, uint16_t sid, uint16_t did, uint8_t* buffer, int buffer_size) {
    // Clear to zeros to be safe
    memset(channel, 0, sizeof(l2cap_channel_t));

    channel->device = device;
    channel->sid = sid;
    channel->did = did;

    channel->on_complete_packet = xSemaphoreCreateCountingStatic(0xFFFF, 0, &channel->semaphore_data[0]);
    
    channel->buffer = buffer;
    channel->buffer_length = buffer_size;

    channel->signal_waiter = xSemaphoreCreateCountingStatic(1, 0, &channel->semaphore_data[1]);
}

int l2cap_open_device(l2cap_device_t* device_handle, uint16_t hci_device_handle) {
    // Set data struct to zero to be fast and easy
    memset(device_handle, 0, sizeof(*device_handle));

    // Set device handle before we add it. Or else...
    device_handle->lock = xSemaphoreCreateMutexStatic(&device_handle->semaphore_data);
    device_handle->handle = hci_device_handle;

    // We dont add channel 0 the traditional way, because its already open by default
    device_handle->open_channels.array = malloc(sizeof(l2cap_channel_t*));
    if(device_handle->open_channels.array == NULL) {
        return BLERROR_OUT_OF_MEMORY;
    }

    device_handle->open_channels.length = 1;
    device_handle->open_channels.array[0] = &device_handle->signal_channel;

    l2cap_channel_t* sig_channel = device_handle->open_channels.array[0];

    // Configure channel
    l2cap_initialize_channel(device_handle, sig_channel, L2CAP_CHANNEL_SIGNALS, L2CAP_CHANNEL_SIGNALS,
        device_handle->signal_channel_buffer, sizeof(device_handle->signal_channel_buffer));
    l2cap_set_channel_receive_event(sig_channel, l2cap_on_received_signal, NULL);

    // Add this to currently open channels
    l2cap_push_device_list(device_handle);

    return 0;
}

void l2cap_close_device(l2cap_device_t* device_handle) {

}

// Request to configure a channel
// Needed since you can't just set the MTU and flush interval from the
// configuration response,
// But this must be set for wiimotes to actually send reports.
static int l2cap_channel_configuration_request(l2cap_channel_t* channel, uint16_t mtu, uint16_t flush_timeout) {
    l2cap_device_t* device_handle = (l2cap_device_t*)channel->device;
    l2cap_signal_t sig;

    sig.code = L2CAP_SIGNAL_CODE_CONFIGURE_REQUEST;
    sig.length = 12;

    // Destination + Flags
    sig.data[0] = channel->did & 0xFF;
    sig.data[1] = channel->did >> 8;
    sig.data[2] = 0x00; sig.data[3] = 0x00; // Flags

    // MTU = 672
    sig.data[4] = 0x01; sig.data[5] = 0x02;
    sig.data[6] = mtu & 0xFF; sig.data[7] = mtu >> 8;

    // Flush Timeout = 0xFFFF (Infinite)
    sig.data[8] = 0x02; sig.data[9] = 0x02;
    sig.data[10] = flush_timeout & 0xFF; sig.data[11] = flush_timeout >> 8;

    l2cap_channel_t* signal_channel = device_handle->open_channels.array[0];

    int ret = l2cap_send_signal(signal_channel, &sig, L2CAP_SIGNAL_CODE_CONFIGURE_RESPONSE);
    if(ret < 0)
        return ret;
    
    uint16_t status = (uint16_t)sig.data[4] | ((uint16_t)sig.data[5] << 8);

    if(status != 0) {
        L2CAP_LOG_ERROR("Configuration Request Rejected: %04X", status);
        return BLERROR_RUNTIME;
    }

    return 0;
}

int l2cap_open_channel(l2cap_device_t* device_handle, l2cap_channel_t* channel, uint16_t protocol_id, uint8_t* rx_buffer, int rx_buffer_size) {
    l2cap_signal_t sig;

    uint16_t assigned_sid = l2cap_next_free_scid(device_handle);
    ASSERT(assigned_sid); // Ya if this fails thats really weird.

    sig.code = L2CAP_SIGNAL_CODE_CONNECTION_REQUEST;
    sig.length = 4;
    sig.data[0] = protocol_id & 0xFF;
    sig.data[1] = protocol_id >> 8;
    sig.data[2] = assigned_sid & 0xFF;
    sig.data[3] = assigned_sid >> 8;

    l2cap_channel_t* signal_channel = device_handle->open_channels.array[0];

    int ret = l2cap_send_signal(signal_channel, &sig, L2CAP_SIGNAL_CODE_CONNECTION_RESPONSE);
    if(ret < 0)
        return ret;

    // The response handler has already checked the size for this packet

    uint16_t status = (uint16_t)sig.data[6] | ((uint16_t)sig.data[7] << 8);

    // Lookup result
    if(status > 0x0005) status = 0x0005;

    int results[] = {
        0, BLERROR_RUNTIME, BLERROR_CONNECTION_REFUSED_BAD_PSM,
        BLERROR_CONNECTION_REFUSED_SECURITY, BLERROR_CONNECTION_REFUSED_NO_RESOURCED,
        BLERROR_CONNECTION_REFUSED_UNKNOWN
    };

    ret = results[status];
    if(ret < 0)
        return ret;
    
    // Grab the destination ID for this new channel
    uint16_t assigned_did = ((uint16_t)sig.data[0]) >> ((uint16_t)sig.data[1] << 8);

    // Lock the device while adding channel
    xSemaphoreTake(device_handle->lock, portMAX_DELAY);
    
    // Alright if we got this far. Lets make that channel
    l2cap_initialize_channel(device_handle, channel, assigned_sid, assigned_did, rx_buffer, rx_buffer_size);
    l2cap_push_channel(channel);

    L2CAP_LOG_DEBUG("Added channed SID: %04X, DID: %04X", channel->sid, channel->did);

    xSemaphoreGive(device_handle->lock);

    // You will need to configure the channel too for it to work
    ret = l2cap_channel_configuration_request(channel, L2CAP_DEFAULT_MTU, L2CAP_DEFAULT_FLUSH_TIMEOUT);
    if(ret < 0)
        return ret;
    return 0;
}

void l2cap_close_channel(l2cap_channel_t* channel) {

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
    if(fifo_available == 0)
        return 0;
    
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

void l2cap_task(void* unused_1) {
    while(l2cap_state.task_running) {
        // Read ACL Packet
        uint16_t handle_in;
        hci_acl_packet_boundary_flag_t pb_in;
        hci_acl_packet_broadcast_flag_t bc_in;
        uint16_t length_in;
        int ret = hci_receive_acl(&handle_in, &pb_in, &bc_in, &length_in);
        if(ret < 0) {
            L2CAP_LOG_ERROR("Failed to receive ACL.");
            continue;
        };

        // Make sure we know how to process it
        if((pb_in != HCI_ACL_PACKET_BOUNDARY_FLAG_FIRST_AUTOMATICALLY_FLUSHABLE_PACKET && pb_in != HCI_ACL_PACKET_BOUNDARY_FLAG_CONTINUING_FRAGMENT) ||
            bc_in != HCI_ACL_PACKET_BROADCAST_FLAG_PTP) {
            
            L2CAP_LOG_ERROR("L2CAP is not currently programmed to handle PB: %d, BC: %d", pb_in, bc_in);
            continue;
        }

        /// TODO: Do you really need to look this up EVERY time
        // Find out handle
        l2cap_device_t* device = l2cap_find_device_by_hci_handle(handle_in);
        if(device == NULL) {
            L2CAP_LOG_ERROR("Got packet for handle thats not open, %04X", handle_in);
            continue;
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
                continue;
            }

            uint16_t packet_size = ((uint16_t)hci_acl_packet_in.data[0]) | ((uint16_t)hci_acl_packet_in.data[1] << 8);
            uint16_t packet_channel = ((uint16_t)hci_acl_packet_in.data[2]) | ((uint16_t)hci_acl_packet_in.data[3] << 8);

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

                l2cap_task_write_channel_buffer(channel, hci_acl_packet_in.data + 4, length_in - 4);
                l2cap_task_update_remaining(device, length_in - 4);
            }

            // Alert thread if this was the whole packet
            if(device->reading_remaining == 0) {
                l2cap_task_write_channel_end_of_packet(channel);
                channel_on_receive_event_queued = true;
            }
        } else {
            // It needs to be open
            if(device->reading_channel == NULL || device->reading_channel->buffer == NULL) {
                xSemaphoreGive(device->lock);
                continue;
            }

            // Write data
            l2cap_task_write_channel_buffer(channel, hci_acl_packet_in.data, length_in);
            l2cap_task_update_remaining(device, length_in);

            // Alert thread if this was the whole packet
            if(device->reading_remaining == 0) {
                l2cap_task_write_channel_end_of_packet(channel);
                channel_on_receive_event_queued = true;
            }
        }

        xSemaphoreGive(device->lock);

        if(channel_on_receive_event_queued && channel->event_packet_available.event) {
            channel->event_packet_available.event(channel, channel->event_packet_available.data);
        }
    }

    xSemaphoreGive(l2cap_state.waiter);

    L2CAP_LOG_INFO("Task Exiting...");
    // Return from task into infinite loop.
    // Because this FreeRTOSs port supports that
}