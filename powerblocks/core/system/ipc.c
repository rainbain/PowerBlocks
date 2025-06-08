/**
 * @file ipc.c
 * @brief Inter Processor Communications
 *
 * IPC interface between Broadway and Starlet.
 * Designed to implement IOS's protocol.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "ipc.h"

#include "system/system.h"
#include "system/exceptions.h"

#include "utils/log.h"

static const char* TAB = "IPC";

#define IPC_PPCMSG   (*(volatile uint32_t*)0xcd800000)
#define IPC_PPCCTRL  (*(volatile uint32_t*)0xcd800004)
#define IPC_ARMMSG   (*(volatile uint32_t*)0xcd800008)
#define IPC_ARMCTRL  (*(volatile uint32_t*)0xcd80000c)

// An arbitrary value, but it will stop putting in request if this many are pending
#define IPC_MAX_REQUEST_COUNT 32

#define IPC_PPCCTRL_X1  (1<<0)
#define IPC_PPCCTRL_Y2  (1<<1)
#define IPC_PPCCTRL_Y1  (1<<2)
#define IPC_PPCCTRL_X2  (1<<3)
#define IPC_PPCCTRL_IY1 (1<<4)
#define IPC_PPCCTRL_IY2 (1<<5)

#define IPC_MESSAGE_MAGIC 0x64e0eaed

static StaticSemaphore_t ipc_semaphore_static[2];

// Taken when something is sending an IPC command and awaiting an acknowledgment.
// So other task will wait for it to be done before using it.
static SemaphoreHandle_t ipc_semaphore_busy;

// A counting semaphore thats givin when a interrupt is
// generated carrying a acknowledgement. Allowing the thread to continue.
static SemaphoreHandle_t ipc_semaphore_acknowledge;

static void ipc_irq_handler(exception_irq_type_t irq) {
    uint32_t ctrl = IPC_PPCCTRL;

    if(ctrl & IPC_PPCCTRL_Y2) {
        xSemaphoreGiveFromISR(ipc_semaphore_acknowledge, &exception_isr_context_switch_needed);
        IPC_PPCCTRL = (IPC_PPCCTRL & 0x30) | IPC_PPCCTRL_Y2; // Clear bit
    }

    if(ctrl & IPC_PPCCTRL_Y1) {
        ipc_message* message = (ipc_message*)SYSTEM_MEM_CACHED(IPC_ARMMSG);

        if(message->magic == IPC_MESSAGE_MAGIC) {
            xSemaphoreGiveFromISR(message->respose, &exception_isr_context_switch_needed);
        }

        IPC_PPCCTRL = (IPC_PPCCTRL & 0x30) | IPC_PPCCTRL_Y1 | IPC_PPCCTRL_X2; // Clear bit and relaunch
    }

    // Acknowledge interrupts.
    // Must happen after so previous bit flips do not make interrupts.
    EXCEPTION_PPC_IRQ |= (1<<30);
}

void ipc_initialize() {
    // Create Semaphores
    ipc_semaphore_busy = xSemaphoreCreateMutexStatic(ipc_semaphore_static + 0);
    ipc_semaphore_acknowledge = xSemaphoreCreateCountingStatic(IPC_MAX_REQUEST_COUNT, 0, ipc_semaphore_static + 1);

    // Register and Enable Interrupts
    exceptions_install_irq(ipc_irq_handler, EXCEPTION_IRQ_TYPE_IPC);
    IPC_PPCCTRL = IPC_PPCCTRL_IY1 | IPC_PPCCTRL_IY2;

    LOG_INFO(TAB, "IPC initialized.");
}

int ipc_request(ipc_message* message) {
    // Treat the message as volatile
    volatile ipc_message* message_v = (volatile ipc_message*)message;

    // Create the semaphore
    message_v->respose = xSemaphoreCreateCountingStatic(10, 0, (StaticSemaphore_t*)&message_v->semaphore_data);

    // Set the Magic
    message_v->magic = IPC_MESSAGE_MAGIC;

    // Flush cache for message so starlet can see it
    // Only do the 32 bytes, since thats only the part starlet cares about
    system_flush_dcache((void*)message_v, 32);

    // Send request to hardware
    xSemaphoreTake(ipc_semaphore_busy, portMAX_DELAY);
    IPC_PPCMSG = SYSTEM_MEM_PHYSICAL(message_v);
    IPC_PPCCTRL = (IPC_PPCCTRL & 0x30) | IPC_PPCCTRL_X1;

    // Wait for acknowledgment.
    xSemaphoreTake(ipc_semaphore_acknowledge, portMAX_DELAY);
    xSemaphoreGive(ipc_semaphore_busy);

    // Wait for response
    xSemaphoreTake((SemaphoreHandle_t)message_v->respose, portMAX_DELAY);

    // Invalidate memory so we can read the return value
    system_invalidate_dcache((void*)message_v, 32);
    int result = message_v->returned;

    // Clear magic
    message_v->magic = 0;

    return result;
}