/**
 * @file video.c
 * @brief Manages the video output of the system.
 *
 * Interacts and initializes the video interface.
 * Has functions for working with VSync, creating and using framebuffers.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "video.h"

#include "system/system.h"
#include "system/exceptions.h"
#include "system/ios_settings.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "utils/log.h"

#include <string.h>

static const char* TAB = "VIDEO";

#define VI_BASE 0xcc002000

#define VI_VTR  (*(volatile uint16_t*)0xcc002000)
#define VI_DCR  (*(volatile uint16_t*)0xCC002002)
#define VI_HTR0 (*(volatile uint32_t*)0xCC002004)
#define VI_HTR1 (*(volatile uint32_t*)0xCC002008)
#define VI_VTO  (*(volatile uint32_t*)0xCC00200C)
#define VI_VTE  (*(volatile uint32_t*)0xCC002010)
#define VI_BBEI (*(volatile uint32_t*)0xCC002014)
#define VI_BBOI (*(volatile uint32_t*)0xCC002018)
#define VI_TFBL (*(volatile uint32_t*)0xCC00201c)
#define VI_TFBR (*(volatile uint32_t*)0xCC002020)
#define VI_BFBL (*(volatile uint32_t*)0xCC002024)
#define VI_BFBR (*(volatile uint32_t*)0xCC002028)
#define VI_DPV  (*(volatile uint16_t*)0xCC00202C)

#define VI_DI0  (*(volatile uint32_t*)0xCC002030)
#define VI_DI1  (*(volatile uint32_t*)0xCC002034)
#define VI_DI2  (*(volatile uint32_t*)0xCC002038)
#define VI_DI3  (*(volatile uint32_t*)0xCC00203C)

#define VI_DTV  (*(volatile uint32_t*)0xCC00206E)

#define VI_DCR_ENABLE      (1<<0)
#define VI_DCR_RESET       (1<<1)
#define VI_DCR_PROGRESSIVE (1<<2)
#define VI_DCR_3D_MODE     (1<<3)
#define VI_DCR_LATCH_0(x)  (((x) & 0b11) << 4)
#define VI_DCR_LATCH_1(x)  (((x) & 0b11) << 6)
#define VI_DCR_FORMAT(x)   (((x) & 0b11) << 8)

#define VI_DTV_GPIO_COMPONENT_CABLE (1<<0)

#define VI_DCR_LATCH_OFF   0
#define VI_DCR_LATCH_PAL   1
#define VI_DCR_LATCH_MPAL  2
#define VI_DCR_LATCH_DEBUG 3

#define VI_DCR_FORMAT_NTSC     0
#define VI_DCR_FORMAT_PAL      1
#define VI_DCR_FORMAT_MPAL     2
#define VI_DCR_FORMAT_RESERVED 3

#define VI_DI_STATUS  (1<<31)
#define VI_DI_ENABLE  (1<<28)


static video_mode_t video_mode = VIDEO_MODE_UNINITIALIZED;

static StaticSemaphore_t video_retrace_semaphore_static;
static SemaphoreHandle_t video_retrace_semaphore; 
static video_retrace_callback_t video_retrace_callback;

// VI States taken from BootMii
/// TODO: BEFORE RELEASE - Make these dynamic.
static const uint16_t VIDEO_Mode640X480NtsciYUV16[64] = {
    0x0F06, 0x0001, 0x4769, 0x01AD, 0x02EA, 0x5140, 0x0003, 0x0018,
    0x0002, 0x0019, 0x410C, 0x410C, 0x40ED, 0x40ED, 0x0043, 0x5A4E,
    0x0000, 0x0000, 0x0043, 0x5A4E, 0x0000, 0x0000, 0x0000, 0x0000,
    0x1107, 0x01AE, 0x1001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0000, 0x0000, 0x0000, 0x0000, 0x2850, 0x0100, 0x1AE7, 0x71F0,
    0x0DB4, 0xA574, 0x00C1, 0x188E, 0xC4C0, 0xCBE2, 0xFCEC, 0xDECF,
    0x1313, 0x0F08, 0x0008, 0x0C0F, 0x00FF, 0x0000, 0x0000, 0x0000,
    0x0280, 0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF};
  
  static const uint16_t VIDEO_Mode640X480Pal50YUV16[64] = {
    0x11F5, 0x0101, 0x4B6A, 0x01B0, 0x02F8, 0x5640, 0x0001, 0x0023,
    0x0000, 0x0024, 0x4D2B, 0x4D6D, 0x4D8A, 0x4D4C, 0x0043, 0x5A4E,
    0x0000, 0x0000, 0x0043, 0x5A4E, 0x0000, 0x0000, 0x013C, 0x0144,
    0x1139, 0x01B1, 0x1001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0000, 0x0000, 0x0000, 0x0000, 0x2850, 0x0100, 0x1AE7, 0x71F0,
    0x0DB4, 0xA574, 0x00C1, 0x188E, 0xC4C0, 0xCBE2, 0xFCEC, 0xDECF,
    0x1313, 0x0F08, 0x0008, 0x0C0F, 0x00FF, 0x0000, 0x0000, 0x0000,
    0x0280, 0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF};
  
  static const uint16_t VIDEO_Mode640X480Pal60YUV16[64] = {
    0x0F06, 0x0001, 0x4769, 0x01AD, 0x02EA, 0x5140, 0x0003, 0x0018,
    0x0002, 0x0019, 0x410C, 0x410C, 0x40ED, 0x40ED, 0x0043, 0x5A4E,
    0x0000, 0x0000, 0x0043, 0x5A4E, 0x0000, 0x0000, 0x0005, 0x0176,
    0x1107, 0x01AE, 0x1001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0000, 0x0000, 0x0000, 0x0000, 0x2850, 0x0100, 0x1AE7, 0x71F0,
    0x0DB4, 0xA574, 0x00C1, 0x188E, 0xC4C0, 0xCBE2, 0xFCEC, 0xDECF,
    0x1313, 0x0F08, 0x0008, 0x0C0F, 0x00FF, 0x0000, 0x0000, 0x0000,
    0x0280, 0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF};
  
  static const uint16_t VIDEO_Mode640X480NtscpYUV16[64] = {
    0x1E0C, 0x0005, 0x4769, 0x01AD, 0x02EA, 0x5140, 0x0006, 0x0030,
    0x0006, 0x0030, 0x81D8, 0x81D8, 0x81D8, 0x81D8, 0x0015, 0x77A0,
    0x0000, 0x0000, 0x0015, 0x77A0, 0x0000, 0x0000, 0x022A, 0x01D6,
    0x120E, 0x0001, 0x1001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0000, 0x0000, 0x0000, 0x0000, 0x2828, 0x0100, 0x1AE7, 0x71F0,
    0x0DB4, 0xA574, 0x00C1, 0x188E, 0xC4C0, 0xCBE2, 0xFCEC, 0xDECF,
    0x1313, 0x0F08, 0x0008, 0x0C0F, 0x00FF, 0x0000, 0x0001, 0x0001,
    0x0280, 0x807A, 0x019C, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF};

static const video_profile_t VIDEO_Profile640X480Ntsci = {
    .width = 640,
    .efb_height = 480,
    .xfb_height = 480,

    .copy_pattern = {
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
    },
    .copy_filer = {0, 0, 21, 22, 21, 0, 0}
    
};

static const video_profile_t VIDEO_Profile640X480Pal50 = {
    .width = 640,
    .efb_height = 480,
    .xfb_height = 576,

    .copy_pattern = {
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
    },
    .copy_filer = {0, 0, 21, 22, 21, 0, 0}
};

static const video_profile_t VIDEO_Profile640X480Pal60 = {
    .width = 640,
    .efb_height = 480,
    .xfb_height = 576,

    .copy_pattern = {
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
    },
    .copy_filer = {0, 0, 21, 22, 21, 0, 0}
};

static const video_profile_t VIDEO_Profile640X480Ntscp = {
    .width = 640,
    .efb_height = 480,
    .xfb_height = 480,

    .copy_pattern = {
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
        {6, 6}, {6, 6}, {6, 6},
    },
    .copy_filer = {0, 0, 21, 22, 21, 0, 0}
};

static void video_irq_handler(exception_irq_type_t irq) {
    uint32_t display;

    // Acknowledge and clear interrupts for the 4 displays.
    display = VI_DI0;
    if(display & VI_DI_STATUS) {
        VI_DI0 = display & ~VI_DI_STATUS;
    }

    display = VI_DI1;
    if(display & VI_DI_STATUS) {
        VI_DI1 = display & ~VI_DI_STATUS;
    }

    display = VI_DI2;
    if(display & VI_DI_STATUS) {
        VI_DI2 = display & ~VI_DI_STATUS;
    }

    display = VI_DI3;
    if(display & VI_DI_STATUS) {
        VI_DI3 = display & ~VI_DI_STATUS;
    }

    // Alert waiting task of vsync
    xSemaphoreGiveFromISR(video_retrace_semaphore, &exception_isr_context_switch_needed);

    if(video_retrace_callback != NULL) {
        video_retrace_callback();
    }
}

video_mode_t video_system_default_video_mode() {
    const char* tv_type = ios_settings_get("VIDEO");
    bool is_progressive_scan = ios_config_is_progressive_scan();
    bool uses_component_cable = VI_DTV = VI_DTV_GPIO_COMPONENT_CABLE;

    if(tv_type == NULL) {
        LOG_ERROR(TAB, "Failed to get TV type setting.\n");
        return VIDEO_MODE_UNINITIALIZED;
    }

    bool use_progressive = is_progressive_scan && uses_component_cable;

    // Very messy, my eyes hurt looking at this.
    if(use_progressive) {
        if(strcmp(tv_type, "NTSC") == 0) {
            return VIDEO_MODE_640X480_NTSC_PROGRESSIVE;
        } else if(strcmp(tv_type, "PAL") == 0) {
            if(ios_config_is_eurgb60()) {
                return VIDEO_MODE_640X480_PAL60;
            } else {
                return VIDEO_MODE_640X480_PAL50;
            }
        } else if(strcmp(tv_type, "MPAL") == 0) {
            return VIDEO_MODE_640X480_PAL50;
        } else {
            LOG_ERROR(TAB, "Unknown TV type %s.", tv_type);
            return VIDEO_MODE_UNINITIALIZED;
        }
    } else {
        if(strcmp(tv_type, "NTSC") == 0) {
            return VIDEO_MODE_640X480_NTSC_INTERLACED;
        } else if(strcmp(tv_type, "PAL") == 0) {
            if(ios_config_is_eurgb60()) {
                return VIDEO_MODE_640X480_PAL60;
            } else {
                return VIDEO_MODE_640X480_PAL50;
            }
        } else if(strcmp(tv_type, "MPAL") == 0) {
            return VIDEO_MODE_640X480_PAL50;
        } else {
            LOG_ERROR(TAB, "Unknown TV type %s.", tv_type);
            return VIDEO_MODE_UNINITIALIZED;
        }
    }
}

const video_profile_t* video_get_profile(video_mode_t mode) {
    switch(mode) {
        case VIDEO_MODE_640X480_NTSC_INTERLACED:
            return &VIDEO_Profile640X480Ntsci;
        case VIDEO_MODE_640X480_NTSC_PROGRESSIVE:
            return &VIDEO_Profile640X480Ntscp;
        case VIDEO_MODE_640X480_PAL50:
            return &VIDEO_Profile640X480Pal50;
        case VIDEO_MODE_640X480_PAL60:
            return &VIDEO_Profile640X480Pal60;
        default:
            LOG_ERROR(TAB, "Invalid video mode %d", mode);
            return NULL;
    }
}

void video_initialize(video_mode_t mode) {
    const uint16_t* vi_state;

    // Create semaphore for retrace (vsync)
    video_retrace_semaphore = xSemaphoreCreateBinaryStatic(&video_retrace_semaphore_static);

    // Enter safe mode when initializing hardware.
    uint32_t irq_enabled;
    SYSTEM_DISABLE_ISR(irq_enabled);

    // Clear callback
    video_retrace_callback = NULL;

    switch(mode) {
        case VIDEO_MODE_640X480_NTSC_INTERLACED:
            vi_state = VIDEO_Mode640X480NtsciYUV16;
            break;
        case VIDEO_MODE_640X480_NTSC_PROGRESSIVE:
            vi_state = VIDEO_Mode640X480NtscpYUV16;
            break;
        case VIDEO_MODE_640X480_PAL50:
            vi_state = VIDEO_Mode640X480Pal50YUV16;
            break;
        case VIDEO_MODE_640X480_PAL60:
            vi_state = VIDEO_Mode640X480Pal60YUV16;
            break;
        default:
            LOG_ERROR(TAB, "Invalid video mode %d", mode);
            return;
    }

    // Reset VI.
    VI_DCR = VI_DCR_RESET;
    system_delay_int(SYSTEM_US_TO_TICKS(10));
    VI_DCR = 0;

    // Copy in Settings
    for(int i = 0; i < 64; i++) {
        volatile uint16_t* reg = (volatile uint16_t*)(VI_BASE + i * 2);
        if(i == 1) {
            // Make sure to clear the enable bit on VI_DCR.
            // We will do that after this copy
            *reg = vi_state[i] & ~(VI_DCR_ENABLE);
        } else {
            *reg = vi_state[i];
        }
    }

    // Finally, enable the interface.
    VI_DCR = vi_state[1];

    video_mode = mode;

    // Register interrupt
    exceptions_install_irq(video_irq_handler, EXCEPTION_IRQ_TYPE_VIDEO);

    LOG_INFO(TAB, "Video interface initialized.");

    SYSTEM_ENABLE_ISR(irq_enabled);
}

void video_set_framebuffer(const framebuffer_t* framebuffer) {
    // Enter safe mode
    uint32_t irq_enabled;
    SYSTEM_DISABLE_ISR(irq_enabled);


    uint32_t fb_address = SYSTEM_MEM_PHYSICAL(framebuffer->pixels);

    uint32_t feild_1 = fb_address;
    uint32_t feild_2 = fb_address;

    if(video_mode != VIDEO_MODE_640X480_NTSC_PROGRESSIVE) {
        // Everything but progressive is interlaced. So the second feild will need to be moved 1 row down.
        feild_2 += VIDEO_WIDTH * 2;
    }

    // Setting bit 28 makes it so that its (address >> 5) giving the full range of addresses.
    VI_TFBL = (feild_1 >> 5) | 0x10000000;
    VI_BFBL = (feild_2 >> 5) | 0x10000000;

    SYSTEM_ENABLE_ISR(irq_enabled);
}

framebuffer_t* video_get_framebuffer() {
    uint32_t address = VI_TFBL;

    address &= 0x0FFFFFFF;
    address <<= 5;

    address = SYSTEM_MEM_CACHED(address);

    return (framebuffer_t*)address;
}

void video_wait_vsync() {
    // Wait till vsync is alerted
    xSemaphoreTake(video_retrace_semaphore, portMAX_DELAY);   
}

void video_wait_vsync_int() {
    while(VI_DPV >= 200);
    while(VI_DPV < 200);
}

void video_set_retrace_callback(video_retrace_callback_t callback) {
    video_retrace_callback = callback;
}