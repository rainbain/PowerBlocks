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
#define BLTOOLS_INFO_LOGGING
#define BLTOOLS_DEBUG_LOGGING

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

    TickType_t remaining_duration;

    // Discovery time tracking
    TickType_t discovery_start;
    
    // Static Data
    bltools_discovery_task_queue_item_t queue_storage_buffer[BLTOOLS_DISCOVERY_TASK_QUEUE_SIZE];
    StaticQueue_t queue_buffer;
    StaticTask_t task_data;
    StackType_t task_stack[BLTOOLS_TASK_STACK_SIZE];
} bltools_discovery_task_t;

static bluetooth_driver_t* bltools_registered_drivers;
static size_t bltools_registered_driver_count;
static bluetooth_driver_instance_t* bltools_instanced_drivers;
static size_t bltools_instanced_driver_count;
static bltools_discovery_task_t* bltools_discovery_instance;

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

int bltools_initialize() {
    // State
    bltools_registered_drivers = NULL;
    bltools_registered_driver_count = 0;
    bltools_instanced_drivers = NULL;
    bltools_instanced_driver_count = 0;
    bltools_discovery_instance = NULL;

    int ret;

    ret = hci_initialize("/dev/usb/oh1/57e/305");
    if(ret < 0)
        return ret;
    
    ret = l2cap_initialize();
    if(ret < 0)
        return ret;
    
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
        
        if(driver->filter_device(device, device_name))
            return driver;
    }
    return NULL;
}

int bltools_load_driver(const bluetooth_driver_t* driver, const hci_discovered_device_info_t* device) {
    bluetooth_driver_instance_t instance;
    instance.driver_id = driver->driver_id;
    instance.instance = driver->initialize_device(device);

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

int bltools_begin_automatic_discovery(TickType_t duration) {
    if(bltools_discovery_instance != NULL) {
        BLTOOLS_LOG_ERROR("Can not begin discovery, discovery already started.");
        return BLERROR_RUNTIME;
    }

    bltools_discovery_instance = malloc(sizeof(*bltools_discovery_instance));

    if(bltools_discovery_instance == NULL) {
        return BLERROR_OUT_OF_MEMORY;
    }

    memset(bltools_discovery_instance, 0, sizeof(*bltools_discovery_instance));
    bltools_discovery_instance->remaining_duration = duration;

    bltools_discovery_instance->task_queue = xQueueCreateStatic(BLTOOLS_DISCOVERY_TASK_QUEUE_SIZE, sizeof(bltools_discovery_task_queue_item_t),
        (uint8_t*)bltools_discovery_instance->queue_storage_buffer, &bltools_discovery_instance->queue_buffer);

    bltools_discovery_instance->task = xTaskCreateStatic(bltools_discovery_task, TAG, sizeof(bltools_discovery_instance->task_stack),
                      bltools_discovery_instance, BLTOOLS_TASK_PRIORITY, bltools_discovery_instance->task_stack, &bltools_discovery_instance->task_data);
    
    return 0;
}

#define DISCOVERY_TASK_QUEUE_START_DISCOVERY   0
#define DISCOVERY_TASK_QUEUE_DISCOVERED_DEVICE 1
#define DISCOVERY_TASK_QUEUE_DISCOVERY_END     2
#define DISCOVERY_TASK_QUEUE_DISCOVERY_ERROR   3

static void bltools_send_queue(bltools_discovery_task_t *inst, const hci_discovered_device_info_t* discovery, uint8_t message) {
    bltools_discovery_task_queue_item_t item;

    item.message = message;
    if(discovery == NULL) {
        memset(&item.info, 0, sizeof(item.info));
    } else {
        memcpy(&item.info, discovery, sizeof(item.info));
    }

    if(xQueueSend(inst->task_queue, &item, 0) != pdPASS) {
        BLTOOLS_LOG_ERROR("Task queue full. Having to wait.");
        xQueueSend(inst->task_queue, &item, portMAX_DELAY);
    }
}

static void hci_discovered_callback(void* user_data, const hci_discovered_device_info_t* device) {
    bltools_discovery_task_t* inst = (bltools_discovery_task_t*)user_data;

    bltools_send_queue(inst, device, DISCOVERY_TASK_QUEUE_DISCOVERED_DEVICE);
}

static void hci_discovery_complete_callback(void* user_data, uint8_t error) {
    bltools_discovery_task_t* inst = (bltools_discovery_task_t*)user_data;

    // If ended in error. Wait a second before starting anew
    if(error != 0) {
        bltools_send_queue(inst, NULL, DISCOVERY_TASK_QUEUE_DISCOVERY_ERROR);
    } else {
        bltools_send_queue(inst, NULL, DISCOVERY_TASK_QUEUE_DISCOVERY_END);
    }
}

static void bltools_task_start_discovery(bltools_discovery_task_t* inst) {
    uint64_t length_seconds = inst->remaining_duration / configTICK_RATE_HZ;
    if(length_seconds > 0x30) {
        length_seconds = 0x30;
    }

    inst->discovery_start = xTaskGetTickCount();

    int ret;
    ret = hci_begin_discovery(HCI_INQUIRY_MODE_GENERAL_ACCESS, length_seconds, 0,
    hci_discovered_callback, hci_discovery_complete_callback, inst);
}

static void bltools_task_discovered_device(bltools_discovery_task_t* inst, const hci_discovered_device_info_t* discovery) {
    uint8_t name[HCI_MAX_NAME_REQUEST_LENGTH];
    
    int ret = hci_get_remote_name(discovery, name);
    if(ret < 0) {
        return;
    }

    bltools_load_compatable_driver(discovery, (const char*)name);
}

static void bltools_task_restart_discovery(bltools_discovery_task_t* inst) {
    // Wait a half second before restarting
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // Update remaining time
    TickType_t duration = xTaskGetTickCount() - inst->discovery_start;

    // Update remaining
    inst->remaining_duration = inst->remaining_duration > duration ? inst->remaining_duration - duration : 0;

    // Exit if theres no more than a second left.
    if(inst->remaining_duration <= (1000 / portTICK_PERIOD_MS)) {
        inst->remaining_duration = 0;
        return;
    }

    bltools_task_start_discovery(inst);
}

static void bltools_discovery_task(void* instance_v) {
    bltools_discovery_task_t* inst = (bltools_discovery_task_t*)instance_v;

    bltools_discovery_task_queue_item_t item;

    // Initial command to start discovery
    bltools_send_queue(inst, NULL, DISCOVERY_TASK_QUEUE_START_DISCOVERY);

    while(true) {
        xQueueReceive(inst->task_queue, &item, portMAX_DELAY);

        switch(item.message) {
            case DISCOVERY_TASK_QUEUE_START_DISCOVERY:
                bltools_task_start_discovery(inst);
                break;
            
            case DISCOVERY_TASK_QUEUE_DISCOVERED_DEVICE:
                bltools_task_discovered_device(inst, &item.info);
                break;
            case DISCOVERY_TASK_QUEUE_DISCOVERY_ERROR:
                BLTOOLS_LOG_ERROR("Discovery error, Restarting Discovery.");
                // No break, keep going

            case DISCOVERY_TASK_QUEUE_DISCOVERY_END:
                // Wait a half second before restarting
                bltools_task_restart_discovery(inst);
                break;
            default:
                BLTOOLS_LOG_ERROR("Unknown discovery queue message: %d", item.message);
        }
    }
}