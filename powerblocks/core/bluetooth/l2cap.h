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

#define L2CAP_DEFAULT_MTU           185
#define L2CAP_DEFAULT_FLUSH_TIMEOUT 0xFFFF

#define L2CAP_CHANNEL_STATUS_OPEN              (1<<0)
#define L2CAP_CHANNEL_STATUS_LOCAL_CONFIGURED  (1<<1) // We have configured the channel
#define L2CAP_CHANNEL_STATUS_REMOTE_CONFIGURED (1<<2) // The remote has configured the channel
#define L2CAP_CHANNEL_STATUS_ERROR             (1<<3)

typedef void (*l2cap_channel_event_t)(void* channel, void* user);
typedef void (*l2cap_disconnect_handler_t)(void* user, uint8_t reason);

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
    uint16_t protocol_id; // The protocol this channel is


    uint16_t status_flags; // Channel flags set by us to track status
    uint16_t status_error; // Error state from a signal.

    SemaphoreHandle_t on_complete_packet;
    SemaphoreHandle_t on_status_change;
    SemaphoreHandle_t status_lock;

    // Stores multiple packets, each starting with a 16 byte size, then data.
    uint8_t* buffer;
    int buffer_length;
    int fifo_write_head;
    int fifo_write_head_packet; // Up to the last packet.
    int fifo_read_head;

    // Event for when a new received packet is made available.
    struct {
        l2cap_channel_event_t event;
        void* data;
    } event_packet_available;

    StaticSemaphore_t semaphore_data[3];
} l2cap_channel_t;

typedef struct {
    SemaphoreHandle_t lock;
    uint16_t handle;
    struct {
        l2cap_channel_t* array;
        size_t length;
    } open_channels;

    // L2CAP Reading into Handle State
    l2cap_channel_t* reading_channel;
    uint16_t reading_remaining;

    uint8_t mac_address[6];

    // Disconnection handler
    l2cap_disconnect_handler_t disconnect_handler;
    void* disconnect_handler_data;

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
 * @brief Initialize a channel before use with open device.
 * 
 * Sets up a channel, call it to set the parameters of the channels.
 * 
 * @param l2cap_device_t Device handle owning the channels
 * @param l2cap_channel_t Channel instance to configure
 * @param sid Source id of the channel, this is your id to reference the channel.
 * @param did Destination id, usually assigned on connect, but is 0x0001 for the signal channel where its expected.
 * @param protocol_id The socket/protocol this channel will handle. Each protocol has its own
 * @param buffer Channel data storage buffer.
 * @param buffer_size Channel data storage buffer size.
 * 
 * @return Negative if Error.
 */
extern void l2cap_initialize_channel(l2cap_device_t* device, l2cap_channel_t* channel, uint16_t sid, uint16_t did, uint16_t protocol_id, uint8_t* buffer, int buffer_size);

/**
 * @brief Opens a L2CAP Connections
 * 
 * Sets up L2CAP to communicate with a device.
 * This sets up the default signal channel L2CAP uses,
 * and adds the device to L2CAPS list of active devices.
 * 
 * The signal channel must always be provided as one of the channels.
 * 
 * @param device_handle L2CAP Device Handle To Create
 * @param hci_device_handle Device handle created by hci_create_connection
 * @param mac_address MAC address of the device. Used to prevent duplicate connections.
 * @param channels List of possible channels, setup with initialize_channel
 * @param channel_count Number of channels
 * 
 * @return Negative if Error.
 */
extern int l2cap_open_device(l2cap_device_t* device_handle, uint16_t hci_device_handle, const uint8_t* mac_address, l2cap_channel_t* channels, size_t channel_count);

/**
 * @brief Closes a L2CAP Connection
 * 
 * Frees a L2CAP device and closes and frees all associated open channels.
 * 
 * @param device_handle L2CAP device to close
 */
extern void l2cap_close_device(l2cap_device_t* device_handle);

/**
 * @brief Set a disconnection handler.
 * 
 * Set a function to be called when a device disconnects.
 * Note, this is called AFTER the device is fully disconnected and L2CAP has
 * terminated all connections.
 * 
 * @param device_handle L2CAP device to set it in
 * @param handler Handler to call
 * @param user_data User data to pass to handler
 */
extern void l2cap_set_disconnect_handler(l2cap_device_t* device_handle, l2cap_disconnect_handler_t handler, void* user_data);

/**
 * @brief Opens a L2CAP Channel
 * 
 * Once a device is open, channels can be opened and closed.
 * Channels are much like sockets, and each one has its own protocol.
 * 
 * By default, the signal channel is already open and used by L2CAP.
 * If you attempted to connect to the device, you will need to open the channels.
 * If the device connects to you, it will be opening the channels.
 * 
 * @param device_handle L2CAP device to open the channel on
 * @param channel Channel to open.
 * 
 * @return Negative if Error.
 */
extern int l2cap_open_channel(l2cap_device_t* device_handle, l2cap_channel_t* channel);

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

/**
 * @brief Waits for a channel to enter a specific state.
 * 
 * Will wait until signal timeout for the flag bits to be set.
 * Any status codes returned by the last event will be set in error_code.
 * 
 * @param channel Channel to wait on
 * @param flags Flag bits that must be set
 * @param error_code Returned error code, can be null
 */
int l2cap_wait_channel_status(l2cap_channel_t* channel, uint16_t flags, uint16_t *error_code);