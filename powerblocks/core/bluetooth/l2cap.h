/**
 * @file l2cap.h
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

#pragma once

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

// Every L2CAP device comes with 1 channel open
// That L2CAP uses for opening other channels and such
// So every device comes with the buffer for this.
// Must be a power of 2.
#define L2CAP_SIGNAL_CHANNEL_BUFFER_SIZE 256

#define L2CAP_CHANNEL_SIGNALS 0x0001

#define L2CAP_DEFAULT_MTU           672
#define L2CAP_DEFAULT_FLUSH_TIMEOUT 0xFFFF

typedef void (*l2cap_channel_event_t)(void* channel, void* user);

// Used internally by L2CAP when handling the signal channel.
typedef struct {
    uint8_t code; // Command Code
    uint8_t id; // Unique id for the request
    uint16_t length;

    uint8_t data[16]; // Arbitrary max size
} l2cap_signal_t;

typedef struct {
    void* device; // l2cap_device_t
    uint16_t sid; // Source ID, ID the endpoint will use when talking to us.
    uint16_t did; // Destination ID, ID we use when talking to the endpoint

    SemaphoreHandle_t on_complete_packet;

    // Stores multiple packets, each starting with a 16 byte size, then data.
    uint8_t* buffer;
    int buffer_length;
    int fifo_write_head;
    int fifo_write_head_packet; // Up to the last packet.
    int fifo_read_head;

    // Track incoming signals
    l2cap_signal_t* incoming_signal;
    uint8_t incoming_id;
    SemaphoreHandle_t signal_waiter;

    // Event for when a new received packet is made available.
    struct {
        l2cap_channel_event_t event;
        void* data;
    } event_packet_available;

    StaticSemaphore_t semaphore_data[2];
} l2cap_channel_t;

typedef struct {
    SemaphoreHandle_t lock;
    uint16_t handle;
    struct {
        l2cap_channel_t** array;
        size_t length;
    } open_channels;

    // Default open channel L2CAP must use
    l2cap_channel_t signal_channel;
    uint8_t signal_channel_buffer[L2CAP_SIGNAL_CHANNEL_BUFFER_SIZE];

    // L2CAP Reading into Handle State
    l2cap_channel_t* reading_channel;
    uint16_t reading_remaining;

    StaticSemaphore_t semaphore_data;
} l2cap_device_t;

/**
 * @brief Initalized L2CAP
 * 
 * Its recommended this is called after hci_initialize
 * but before working with any bluetooth devices.
 * 
 * This allows you to use L2CAP once a connection
 * has been established.
 * 
 * @return Negative if Error
*/
extern int l2cap_initialize();

/**
 * @brief Begins closing out of L2CAP
 * 
 * I don't currently have a good way to cancel the blocking
 * ACL read from it, so this will signal the thread to begin
 * exiting. Once HCI is closed, it will actually exist.
*/
extern void l2cap_signal_close();

/**
 * @brief Closes out of L2CAP
 * 
 * Must come after l2cap_signal_close and the HCI is closed.
 * Will wait for the task to exit.
*/
extern void l2cap_close();

/**
 * @brief Opens a L2CAP Connections
 * 
 * Sets up L2CAP to communicate with a device.
 * This sets up the default signal channel L2CAP uses,
 * and adds the device to L2CAPS list of active devices.
 * 
 * @param device_handle L2CAP Device Handle To Create
 * @param hci_device_handle Device handle created by hci_create_connection
 * 
 * @return Negative if Error.
 */
extern int l2cap_open_device(l2cap_device_t* device_handle, uint16_t hci_device_handle);

/**
 * @brief Closes a L2CAP Connection
 * 
 * Frees a L2CAP device and closes and frees all associated open channels.
 * 
 * @param device_handle L2CAP device to close
 */
extern void l2cap_close_device(l2cap_device_t* device_handle);

/**
 * @brief Opens a L2CAP Channel
 * 
 * Once a device is open, channels can be opened and closed.
 * Channels are much like sockets, and each one has its own protocol.
 * 
 * By default, the signal channel is already open and used by L2CAP.
 * 
 * @param device_handle L2CAP device to open the channel on
 * @param channel Channel data structure to write into
 * @param protocol_id Protocol of the channel. Like the socket.
 * @param rx_buffer The receiving buffer for the channel.
 * @param rx_buffer_size Size of the receiving buffer. Must be a power of 2.
 * 
 * @return Negative if Error.
 */
extern int l2cap_open_channel(l2cap_device_t* device_handle, l2cap_channel_t* channel, uint16_t protocol_id, uint8_t* rx_buffer, int rx_buffer_size);

/**
 * @brief Closes a L2CAP Channel
 * 
 * Closes a channel and frees its resources.
 * 
 * This will automatically be done for any remaining open channels when calling
 * l2cap_close_device.
 * 
 * @param channel Channel to close.
 */
extern void l2cap_close_channel(l2cap_channel_t* channel);

/**
 * @brief Sends data over a L2CAP Channel
 * 
 * Sends a buffer over a L2CAP channel.
 * Channels in L2CAP can be thought of as a lot like
 * sockets. One device can run multiple protocols,
 * and we expect to see specific protocols on specific channels.
 * 
 * @param channel Channel to talk to
 * @param data Data Buffer
 * @param size Size of data. Up to 65535.
 * 
 * @return Negative if Error
 */
extern int l2cap_send_channel(l2cap_channel_t* channel, const void* data, uint16_t size);

/**
 * @brief Receives from a L2CAP Channel
 * 
 * Receives data from the internal buffer specified when creating
 * the channel.
 * 
 * The buffer is treated as a FIFO and can hold multiple packets.
 * In the event of an overflow, packet truncation and packet drops are logged
 * and this will just return truncated or 0 byte packets.
 * 
 * Packets should never be truly lost, just that you will have saved up a 
 * bunch of 0 byte packets.
 * 
 * 
 * Will error on incomplete FIFO fragments.
 * 
 * @param channel Channel to talk to
 * @param data Data Buffer
 * @param size Size of buffer. Up to 65535.
 * 
 * @return Total bytes read (even if they did not fit into buffer). Negative if error.
*/
extern int l2cap_receive_channel(l2cap_channel_t* channel, void* data, uint16_t size);

/**
 * @brief Sets the L2CAP on packet receive event.
 * 
 * When a packet is received on a channel,
 * this event will be called to receive it if you please.
 * 
 * @param channel Channel to set it in
 * @param event Event Handler
 * @param param Parameter passed to event.
 */
extern void l2cap_set_channel_receive_event(l2cap_channel_t* channel, l2cap_channel_event_t event, void* param);
