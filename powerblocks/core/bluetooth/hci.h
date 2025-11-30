/**
 * @file hci.h
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
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "powerblocks/core/system/system.h"
#include "powerblocks/core/ios/ios.h"

typedef struct {
    uint8_t address[6];
    uint8_t link_type; // Only set on connection request handler. 0 otherwise
    uint8_t page_scan_repetition_mode;
    uint32_t class_of_device;
    uint16_t clock_offset;

    // True if this is from a connection request event
    // and not from an enquiry.
    bool connection_request;
} hci_discovered_device_info_t;

typedef struct {
    uint16_t acl_data_packet_length;
    uint8_t synchronous_data_packet_length;
    uint16_t total_num_acl_data_packets;         // Total ACL packets the hardware can store.
    uint16_t total_num_synchronous_data_packets; // Total Sync packets the hardware can store.
} hci_buffer_sizes_t;


// Tired of coding
// Felt like being evil with these names
// Feel free to refactor. I am sorry
typedef enum {
    HCI_ACL_PACKET_BOUNDARY_FLAG_FIRST_NON_AUTOMATICALLY_FLUSHABLE_PACKET,
    HCI_ACL_PACKET_BOUNDARY_FLAG_CONTINUING_FRAGMENT,
    HCI_ACL_PACKET_BOUNDARY_FLAG_FIRST_AUTOMATICALLY_FLUSHABLE_PACKET
} hci_acl_packet_boundary_flag_t;

typedef enum {
    HCI_ACL_PACKET_BROADCAST_FLAG_PTP,
    HCI_ACL_PACKET_BROADCAST_FLAG_ACTIVE_DEVICE,
    HCI_ACL_PACKET_BROADCAST_FLAG_PARKED_DEVICE,
} hci_acl_packet_broadcast_flag_t;

// Reasons for rejecting a connection
typedef enum {
    HCI_REJECT_REASON_LIMITED_RESOURCES =    0x0D,
    HCI_REJECT_REASON_SECURITY_REASONS  =    0x0E,
    HCI_REJECT_REASON_UNACCEPTABLE_ADDRESS = 0x0F
} hci_reject_reason_t;

// HCI Defines multiple inquiry modes. We will probably ever use one.
#define HCI_INQUIRY_MODE_GENERAL_ACCESS 0x9E8B33

// Max size of a name from name request
#define HCI_MAX_NAME_REQUEST_LENGTH 254

// Max size of a ACL data section
// This must be at least greater than the max size
// Reported by the hardware. On the wii it reports 339 bytes.
#define HCI_MAX_ACL_DATA_LENGTH 512

// Defines the amount of data on the stack needed for the ACL receive, used to be kept around for async
#define HCI_ACL_RECEIVE_IPC_BUFFER_SIZE (32 + 32 + 32 + 32 + sizeof(ipc_message))

typedef struct {
    uint16_t handle;
    uint16_t size;
    uint8_t data[HCI_MAX_ACL_DATA_LENGTH];
} hci_acl_packet_t;

// Exposed HCI Buffer Sizes
// These are read from the controller when HCI is initialized.
// Used for knowing how big and how many ACL/SCL buffers you can send
extern hci_buffer_sizes_t hci_buffer_sizes;


/**
 * @brief Callback for handling a discovered device during inquiry.
 *
 * @param user_data User-defined context passed to the callback.
 * @param device    Pointer to the discovered device information.
 */
typedef void (*hci_discovered_device_handler)(void* user_data, const hci_discovered_device_info_t* device);

/**
 * @brief Callback invoked when device discovery completes.
 *
 * @param user_data User-defined context passed to the callback.
 * @param error     Error code (0 on success, nonzero on failure).
 */
typedef void (*hci_discovery_complete_handler)(void* user_data, uint8_t error);

/**
 * @brief Callback for handling a connection request
 *
 * @param user_data User-defined context passed to the callback.
 * @param device    Pointer to the device information.
 */
typedef void (*hci_connection_request_handler_t)(void* user_data, const hci_discovered_device_info_t* device);

/**
 * @brief Callback for when a device disconnects.
 *
 * @param user_data User-defined context passed to the callback.
 * @param handle    Disconnection device handle.
 */
typedef void (*hci_disconnection_complete_handler_t)(void* user_data, uint16_t handle, uint8_t reason);


/**
 * @brief Initializes the HCI interface
 * 
 * Initializes the HCI driver opening the USB interface in IOS.
 * 
 * @param device Device path to the USB device. For the wii's built in its "/dev/usb/oh1/57e/305"
 * 
 * @return Negative if Error
 */
extern int hci_initialize(const char* device);

/**
 * @brief Closes the HCI driver.
 * 
 * 
 * Currently, if HCI gets stuck, this function will get stuck waiting for the
 * resource to open up.
 * 
 * @return Negative if Error
 */
extern void hci_close();

/**
 * @brief Resets the HCI Interface
 * 
 * Its recommended to do this to get it into a known state.
 * @return Negative if Error
 */
extern int hci_reset();

/**
 * @brief Set the HCI connection request handler.
 * 
 * Set a function to be called when a connection request event is received.
 * 
 * hci_write_scan_enable with enable_page_scan true must be done in order
 * to see these.
 * 
 * This event is called when a device, like a wiimote, is attempting to reconnect
 * to a paired device.
 * 
 * You must eather accept or reject the connection on another thread with
 * hci_accept_connection or hci_reject_connection.
 * 
 * It is not recommended to write this register with page scan enabled for
 * thread safety.
 * 
 * @param handler Handler to call
 * @param user_data User data to pass to handler
 * 
 * @return Negative if error.
 */
extern void hci_set_connection_request_handler(hci_connection_request_handler_t handler, void* user_data);

/**
 * @brief Set callback for when a device disconnections.
 * 
 * Set a function to cal when a device disconnects.
 * 
 * This is for all devices, and you are provided the handle to know the device.
 * 
 * This is usually routed to L2CAP who can call the handler of the device and close out the connection.
 * 
 * @param handler Handler to call
 * @param user_data User data to pass to handler
 * 
 * @return Negative if error.
 */
extern void hci_set_disconnection_complete_handler(hci_disconnection_complete_handler_t handler, void* user_data);

/**
 * @brief Writes the HCI's page scan register.
 * 
 * enable_inquiry_scan will make it so HCI responds to other device's
 * bluetooth inquiry. Making this HCI appear as a connectable device.
 * 
 * enable_page_scan will make it so that paired devices may attempt to
 * reconnect to HCI. This will take the form of a connection request event.
 * 
 * @param enable_inquiry_scan Appear in device discoveries.
 * @param enable_page_scan Generate connection request events.
 * 
 * @return Negative if Error
 */
extern int hci_write_scan_enable(bool enable_inquiry_scan, bool enable_page_scan);

/**
 * @brief Rejects an incoming connection request.
 * 
 * Rejects an incoming connection request from page scan.
 * This can be for a number of reasons specified by the caller
 * 
 * @param info Device connection request
 * @param reason Reason for rejecting the connection.
 * 
 * @return Negative if Error
 */
extern int hci_reject_connection(const hci_discovered_device_info_t* info, hci_reject_reason_t reason);

/**
 * @brief Accepts an incoming connection request.
 * 
 * Accepts an incoming connection request from page scan.
 * By default you are the device, and the one who created the request is the host.
 * But in the event the connection request is from a reconnecting device,
 * say a wiimote, you will want to role switch as the host.
 * 
 * @param info Device connection request
 * @param role_switch Role switch to host/master.
 * @param handle Output handle. Not NULL!
 */
extern int hci_accept_connection(const hci_discovered_device_info_t* info, bool role_swich, uint16_t* handle);

/**
 * @brief Begins looking for bluetooth devices.
 * 
 * Will begin looking for bluetooth devices.
 * 
 * On emulator, dolphin will report wiimote devices 1 at a time.
 * 
 * Only 1 thing can be running discovery at a time. Calling this while a discovery
 * session is already running will return an error.
 * 
 * You generally need to wait for the inquiry to end before doing anything.
 * ACL packets are fine though.
 * 
 * Handlers called from the HCI Task.
 * Please do not call HCI function from the HCI task.
 * 
 * @param lap Access code for what devices to discover. Usually HCI_INQUIRY_MODE_GENERAL_ACCESS
 * @param length Time to discover devices for. In units of 1.28 seconds. 0x01 – 0x30, Dolphin always completes instantly
 * @param responses Max devices discovered. Or 0 for infinite up until timeout.
 * @param on_discovered Called when a device is discovered. May be NOT be NULL
 * @param on_complete Called when the discovery session ends. May be NULL
 * @param user_data Pointer passed to handlers. May be NULL.
 * 
 * @return Negative if Error
*/
extern int hci_begin_discovery(
        uint32_t lap, uint8_t length, uint8_t responses,
        hci_discovered_device_handler on_discovered, hci_discovery_complete_handler on_complete, void *user_data);

/**
 * @brief Stops a discovery session.
 * 
 * Once returns a non error value,
 * no more of the discovery handlers will be called.
 * 
 * @return Negative if Error
 */
extern int hci_cancel_discovery();

/**
 * @brief Returns the remote name of a device
 * 
 * Useful for for device detection and enumeration.
 * 
 * IMPORTANT: You can't call this during an inquiry, you must
 * wait for it to end.
 * 
 * @param device Device information to poll.
 * @param name Name output, must be HCI_MAX_NAME_REQUEST_LENGTH in size.
 * 
 * @return Negative if Error
 */
extern int hci_get_remote_name(const hci_discovered_device_info_t* device, uint8_t* name);

/**
 * @brief Creates a connection to a device for you to use.
 * 
 * @param device Discovered device to connect to
 * @param handle Device handle out. Not null
 * 
 * @return Negative if Error
 */
extern int hci_create_connection(const hci_discovered_device_info_t* device, uint16_t* handle);

/**
 * @brief Disconnects from a device
 * 
 * @param handle Handle to device
 * 
 * @return Negative if Error
*/
extern int hci_disconnect(uint16_t handle);

// These 2 buffers have been statically allocated so that
// You can lower the amount of memcpys in ACL transactions
extern SemaphoreHandle_t hcl_acl_packet_out_lock;
extern hci_acl_packet_t hci_acl_packet_out ALIGN(32) MEM2;
extern hci_acl_packet_t hci_acl_packet_in_0 ALIGN(32) MEM2;
extern hci_acl_packet_t hci_acl_packet_in_1 ALIGN(32) MEM2;

/**
 * @brief Sends a ACL Packet
 * 
 * These are the asynchronous data packets that are sent to devices.
 * Multiplexing / Reassembly / Channels are handled by L2CAP that makes these
 * 
 * These parameters will be set into hci_acl_packet_out, you must fill
 * in the data though.
 * 
 * @param handle Bluetooth Connection Handle From hci_create_connection
 * @param pb Packet boundary flag.
 * @param bc Broadcast boundary flag.
 * @param length Length of data transmitted.
 * 
 * @return Negative if Error
 */
extern int hci_send_acl(uint16_t handle, hci_acl_packet_boundary_flag_t pb, hci_acl_packet_broadcast_flag_t bc, uint16_t length);

/**
 * @brief Receives a ACL Packet Asynchronously
 * 
 * Unlike send, this is done asynchronously, and returns early,
 * so that you can double buffer and make sure to never miss an ACL packet.
 * Multiplexing / Reassembly / Channels are handled by L2CAP that makes these
 * 
 * These parameters will be set into hci_acl_packet_in.
 * Your data and length is available in there.
 * 
 * Also must be provided is a buffer size HCI_ACL_RECEIVE_IPC_BUFFER_SIZE for storing the IPC
 * data. So that it can store the message in the mean time until the callback is called.
 * 
 * @param buffer ACL packet buffer to receive into.
 * @param ipc_buffer Data buffer for the IPC data. Needs to be kept around until the callback is called. Needs to be 32 byte aligned.
 * @param handler Called when data is received
 * @param params Pointer passed to handler
 * 
 * @return Negative if Error
 */
extern int hci_receive_acl_async(hci_acl_packet_t* acl_buffer, uint8_t* ipc_buffer, ipc_async_handler_t handler, void* params);


/**
 * @brief Decodes a received ACL packet.
 * 
 * Like send, gets back the packet boundary, broadcast flags, and more.
 * 
 * @param handle Bluetooth Connection Handle In. Not NULL!
 * @param pb Packet boundary Flag In. Not NULL!
 * @param bc Broadcast boundary Flag In. Not NULL!
 * @param length Length of data received. Not NULL!
 * 
 * @return Negative if Error
 */
extern void hci_decode_received_acl(uint16_t *handle, hci_acl_packet_boundary_flag_t *pb, hci_acl_packet_broadcast_flag_t *bc, uint16_t* length, const hci_acl_packet_t* acl_buffer);