/**
 * @file bltools.h
 * @brief Bluetooth Tools
 *
 * Implemented for working with bluetooth.
 * Designed to mainstream connecting devices and loading appropriate drivers.
 * The use of this is not required, it just makes using bluetooth easier.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#include "bltootls.h"

#include "powerblocks/core/utils/log.h"


#include "blerror.h"
#include "hci.h"
#include "l2cap.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdlib.h>
#include <string.h>

static const char* TAG = "BLTOOLS";

#define BLTOOLS_TASK_STACK_SIZE  4096
#define BLTOOLS_TASK_PRIORITY    (configMAX_PRIORITIES / 4 * 1)
#define BLTOOLS_DISCOVERY_TASK_QUEUE_SIZE 5

#define BLTOOLS_ERROR_LOGGING
//#define BLTOOLS_INFO_LOGGING
//#define BLTOOLS_DEBUG_LOGGING

#ifdef BLTOOLS_ERROR_LOGGING
#define BLTOOLS_LOG_ERROR(fmt, ...) LOG_ERROR(TAG, fmt, ##__VA_ARGS__)
#else
#define BLTOOLS_LOG_ERROR(fmt, ...)
#endif 

#ifdef BLTOOLS_INFO_LOGGING
#define BLTOOLS_LOG_INFO(fmt, ...) LOG_INFO(TAG, fmt, ##__VA_ARGS__)
#else
#define BLTOOLS_LOG_INFO(fmt, ...)
#endif 

#ifdef BLTOOLS_DEBUG_LOGGING
#define BLTOOLS_LOG_DEBUG(fmt, ...) LOG_DEBUG(TAG, fmt, ##__VA_ARGS__)
#else
#define BLTOOLS_LOG_DEBUG(fmt, ...)
#endif

typedef struct {
    uint8_t message;
    hci_discovered_device_info_t info;
} bltools_discovery_task_queue_item_t;

typedef struct {
    TaskHandle_t task;
    QueueHandle_t task_queue;
    
    // Static Data
    bltools_discovery_task_queue_item_t queue_storage_buffer[BLTOOLS_DISCOVERY_TASK_QUEUE_SIZE];
    StaticQueue_t queue_buffer;
    StaticTask_t task_data;
    StackType_t task_stack[BLTOOLS_TASK_STACK_SIZE];
} bltools_worker_task_t;

static bluetooth_driver_t* bltools_registered_drivers;
static size_t bltools_registered_driver_count;
static bluetooth_driver_instance_t* bltools_instanced_drivers;
static size_t bltools_instanced_driver_count;
static bltools_worker_task_t* bltools_worker_instance;

static void bltools_discovery_task(void* instance_v);

static void bltools_push_driver(bluetooth_driver_instance_t* driver) {
    bluetooth_driver_instance_t* slot = NULL;
    for(size_t i = 0; i < bltools_instanced_driver_count; i++) {
        if(bltools_instanced_drivers[i].driver_id != BLUETOOTH_DRIVER_ID_INVALID) {
            slot = &bltools_instanced_drivers[i];
            break;
        }
    }

    if(slot == NULL) {
        bltools_instanced_driver_count++;
        bltools_instanced_drivers = realloc(bltools_instanced_drivers, sizeof(bluetooth_driver_instance_t) * bltools_instanced_driver_count);
        slot = &bltools_instanced_drivers[bltools_instanced_driver_count-1];

        ASSERT_OUT_OF_MEMORY(bltools_instanced_drivers);
    }

    memcpy(slot, driver, sizeof(bluetooth_driver_instance_t));
}


#define DISCOVERY_TASK_QUEUE_DISCOVERED_DEVICE  1
#define DISCOVERY_TASK_QUEUE_CONNECTION_REQUEST 2

static void bltools_send_queue(bltools_worker_task_t *inst, uint8_t message, const hci_discovered_device_info_t* info) {
    bltools_discovery_task_queue_item_t item;

    item.message = message;
    if(info != NULL) {
        memcpy(&item.info, info, sizeof(item.info));
    } else {
        memset(&item.info, 0, sizeof(item.info));
    }

    if(xQueueSend(inst->task_queue, &item, 0) != pdPASS) {
        BLTOOLS_LOG_ERROR("Task queue full! Dropping message %d", message);
    }
}

static void bltools_hci_discovered_callback(void* user_data, const hci_discovered_device_info_t* device) {
    bltools_worker_task_t* inst = (bltools_worker_task_t*)user_data;
    bltools_send_queue(inst, DISCOVERY_TASK_QUEUE_DISCOVERED_DEVICE, device);
}

static void bltools_hci_connection_request_callback(void* user_data, const hci_discovered_device_info_t* device) {
    bltools_worker_task_t* inst = (bltools_worker_task_t*)user_data;
    bltools_send_queue(inst, DISCOVERY_TASK_QUEUE_CONNECTION_REQUEST, device);
}

static int bltools_start_worker() {
    bltools_worker_instance = malloc(sizeof(*bltools_worker_instance));

    if(bltools_worker_instance == NULL) {
        return BLERROR_OUT_OF_MEMORY;
    }

    memset(bltools_worker_instance, 0, sizeof(*bltools_worker_instance));

    bltools_worker_instance->task_queue = xQueueCreateStatic(BLTOOLS_DISCOVERY_TASK_QUEUE_SIZE, sizeof(bltools_discovery_task_queue_item_t),
        (uint8_t*)bltools_worker_instance->queue_storage_buffer, &bltools_worker_instance->queue_buffer);
    
    bltools_worker_instance->task = xTaskCreateStatic(bltools_discovery_task, TAG, sizeof(bltools_worker_instance->task_stack),
                      bltools_worker_instance, BLTOOLS_TASK_PRIORITY, bltools_worker_instance->task_stack, &bltools_worker_instance->task_data);

    return 0;
}

int bltools_initialize() {
    // State
    bltools_registered_drivers = NULL;
    bltools_registered_driver_count = 0;
    bltools_instanced_drivers = NULL;
    bltools_instanced_driver_count = 0;
    bltools_worker_instance = NULL;

    int ret;

    ret = hci_initialize("/dev/usb/oh1/57e/305");
    if(ret < 0)
        return ret;
    
    ret = l2cap_initialize();
    if(ret < 0)
        return ret;
    
    // Start worker
    bltools_start_worker();

    // Route hci connection request here
    hci_set_connection_request_handler(bltools_hci_connection_request_callback, bltools_worker_instance);
    
    return 0;
}

void bltools_register_driver(bluetooth_driver_t driver) {
    // Make sure we dont have duplicate ID
    if(bltoots_find_driver_by_id(driver.driver_id) != NULL) {
        BLTOOLS_LOG_ERROR("Can not register driver. Driver of ID %d already exist.", driver.driver_id);
        return;
    }

    // Find freeslot to put driver
    bluetooth_driver_t* slot = bltoots_find_driver_by_id(BLUETOOTH_DRIVER_ID_INVALID);

    // Allocate new slot
    if(slot == NULL) {
        bltools_registered_driver_count++;
        bltools_registered_drivers = (bluetooth_driver_t*)realloc(bltools_registered_drivers, sizeof(bluetooth_driver_t) * bltools_registered_driver_count);
        slot = &bltools_registered_drivers[bltools_registered_driver_count-1];

        ASSERT_OUT_OF_MEMORY(bltools_registered_drivers);
    }

    memcpy(slot, &driver, sizeof(bluetooth_driver_t));
}

bluetooth_driver_t* bltoots_find_driver_by_id(uint16_t driver_id) {
    for(size_t i = 0; i < bltools_registered_driver_count; i++) {
        if(bltools_registered_drivers[i].driver_id == driver_id) {
            return &bltools_registered_drivers[i];
        }
    }
    return NULL;
}

bluetooth_driver_t* bltools_find_compatable_driver(const hci_discovered_device_info_t* device, const char* device_name) {
    for(size_t i = 0; i < bltools_registered_driver_count; i++) {
        bluetooth_driver_t* driver = &bltools_registered_drivers[i];
        if(driver->driver_id == BLUETOOTH_DRIVER_ID_INVALID)
            continue;
        
        if(device->connection_request) {
            if(driver->filter_paired_device(device))
                return driver;
        } else {
            if(driver->filter_device(device, device_name))
                return driver;
        }
    }
    return NULL;
}

int bltools_load_driver(const bluetooth_driver_t* driver, const hci_discovered_device_info_t* device) {
    bluetooth_driver_instance_t instance;
    instance.driver_id = driver->driver_id;
    if(device->connection_request) {
        instance.instance = driver->initialize_paired_device(device);
    } else {
        instance.instance = driver->initialize_new_device(device);
    }

    // Do not display as an error, its normal
    // to try and connect to a device and have that fail
    if(instance.instance == NULL) {
        BLTOOLS_LOG_INFO("Failed to initialize driver %d.", driver->driver_id);
        return BLERROR_DRIVER_INITIALIZE_FAIL;
    }

    bltools_push_driver(&instance);
    return 0;
}

int bltools_load_compatable_driver(const hci_discovered_device_info_t* device, const char* device_name) {
    const bluetooth_driver_t* driver = bltools_find_compatable_driver(device, device_name);
    if(driver == NULL) {
        return BLERROR_NO_DRIVER_FOUND;
    }

    return bltools_load_driver(driver, device);
}

int bltools_begin_discovery(uint32_t lap, TickType_t duration, uint8_t responses) {
    // Clamp duration
    if(duration < (1280 / portTICK_RATE_MS))
        duration = 1280 / portTICK_RATE_MS;
    if(duration > (61440 / portTICK_RATE_MS))
        duration = 61440 / portTICK_RATE_MS;
    
    uint8_t length = duration / (1280 / portTICK_RATE_MS);

    return hci_begin_discovery(lap, length, responses, bltools_hci_discovered_callback, NULL, bltools_worker_instance);
}

static void bltools_task_discovered_device(bltools_worker_task_t* inst, const hci_discovered_device_info_t* discovery) {
    uint8_t name[HCI_MAX_NAME_REQUEST_LENGTH];

    int ret = hci_get_remote_name(discovery, name);
    if(ret < 0) {
        return;
    }
    bltools_load_compatable_driver(discovery, (const char*)name);
}

static void bltools_task_paired_device(bltools_worker_task_t* inst, const hci_discovered_device_info_t* discovery) {
    bltools_load_compatable_driver(discovery, NULL);
}

static void bltools_discovery_task(void* instance_v) {
    bltools_worker_task_t* inst = (bltools_worker_task_t*)instance_v;

    bltools_discovery_task_queue_item_t item;

    while(true) {
        xQueueReceive(inst->task_queue, &item, portMAX_DELAY);

        BLTOOLS_LOG_DEBUG("Processing event: %d", item.message);

        switch(item.message) {
            case DISCOVERY_TASK_QUEUE_DISCOVERED_DEVICE:
                bltools_task_discovered_device(inst, &item.info);
                break;
            case DISCOVERY_TASK_QUEUE_CONNECTION_REQUEST:
                bltools_task_paired_device(inst, &item.info);
                break;
            default:
                BLTOOLS_LOG_ERROR("Unknown discovery queue message: %d", item.message);
        }
    }
}