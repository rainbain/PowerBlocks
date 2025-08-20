/**
 * @file gx.h
 * @brief Manages graphics on the system.
 *
 *  Manages graphics on the system. Provides a interface to create and send
 *  commands to hardware, and configure the hardware for rendering.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "gx.h"

#include "system/system.h"

#include "utils/log.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

static const char* TAB = "GX";

// External Command Processor Registers
#define CP_STATUS                 (*(volatile uint16_t*)0xCC000000) // State of the Command Processor
#define CP_CONTROL                (*(volatile uint16_t*)0xCC000002) // Interrupt Control / Enable
#define CP_CLEAR                  (*(volatile uint16_t*)0xCC000004) // Clear Interrupts
#define CP_TOKEN                  (*(volatile uint16_t*)0xCC00000E) // Token Value (Debugging Tools)
//#define CP_BOUNDING_LEFT          (*(volatile uint16_t*)0xCC000010) // Maybe a mistake in yagcd
//#define CP_BOUNDING_RIGHT         (*(volatile uint16_t*)0xCC000012) // Like these look to be in the pixel engine
//#define CP_BOUNDING_TOP           (*(volatile uint16_t*)0xCC000014)
//#define CP_BOUNDING_BOTTOM        (*(volatile uint16_t*)0xCC000016)
#define CP_FIFO_BASE_LOW          (*(volatile uint16_t*)0xCC000020) // Base address of command fifo
#define CP_FIFO_BASE_HIGH         (*(volatile uint16_t*)0xCC000022)
#define CP_FIFO_END_LOW           (*(volatile uint16_t*)0xCC000024) // Address of last word. So
#define CP_FIFO_END_HIGH          (*(volatile uint16_t*)0xCC000026) // address + size - 4.
#define CP_HIGH_WATERMARK_LOW     (*(volatile uint16_t*)0xCC000028) // Interrupt to be asserted when fifo expanding reaches this size
#define CP_HIGH_WATERMARK_HIGH    (*(volatile uint16_t*)0xCC00002a)
#define CP_LOW_WATERMARK_LOW      (*(volatile uint16_t*)0xCC00002c) // Interrupt to be asserted when fifo shrinking reaches this size
#define CP_LOW_WATERMARK_HIGH     (*(volatile uint16_t*)0xCC00002e)
#define CP_FIFO_DISTANCE_LOW      (*(volatile uint16_t*)0xCC000030) // Amount of data in fifo
#define CP_FIFO_DISTANCE_HIGH     (*(volatile uint16_t*)0xCC000032)
#define CP_FIFO_WRITE_HEAD_LOW    (*(volatile uint16_t*)0xCC000034) // Pointer to data written
#define CP_FIFO_WRITE_HEAD_HIGH   (*(volatile uint16_t*)0xCC000036)
#define CP_FIFO_READ_HEAD_LOW     (*(volatile uint16_t*)0xCC000038) // Pointer to data read
#define CP_FIFO_READ_HEAD_HIGH    (*(volatile uint16_t*)0xCC00003a)
#define CP_FIFO_BREAKPOINT_LOW    (*(volatile uint16_t*)0xCC00003c) // Address to trigger breakpoint on
#define CP_FIFO_BREAKPOINT_HIGH   (*(volatile uint16_t*)0xCC00003e)

// Pixel Engine Registers
#define PE_Z_CONFIG               (*(volatile uint16_t*)0xCC001000)
#define PE_ALPHA_CONFIG           (*(volatile uint16_t*)0xCC001002)
#define PE_DESTINATION_ALPHA      (*(volatile uint16_t*)0xCC001004)
#define PE_ALPHA_MODE             (*(volatile uint16_t*)0xCC001006)
#define PE_ALPHA_READ             (*(volatile uint16_t*)0xCC001008)
#define PE_ISR                    (*(volatile uint16_t*)0xCC00100A)
#define PE_TOKEN                  (*(volatile uint16_t*)0xCC00100E)

// Processor Interface Registers
#define PI_FIFO_BASE         (*(volatile uint32_t*)0xCC00300C)
#define PI_FIFO_END          (*(volatile uint32_t*)0xCC003010)
#define PI_FIFO_WRITE_HEAD   (*(volatile uint32_t*)0xCC003014)

#define CP_STATUS_GX_FIFO_OVERFLOW      (1<<0)
#define CP_STATUS_GX_FIFO_UNDERFLOW     (1<<1)
#define CP_STATUS_GP_IDLE_READING       (1<<2)
#define CP_STATUS_GP_IDLE_COMMANDS      (1<<3)
#define CP_STATUS_BREAK_POINT_INTERRUPT (1<<4)

#define CP_CONTROL_FIFO_READ                 (1<<0)
#define CP_CONTROL_IRQ_ENABLE                (1<<1)
#define CP_CONTROL_FIFO_OVERFLOW_IRQ_ENABLE  (1<<2)
#define CP_CONTROL_FIFO_UNDERFLOW_IRQ_ENABLE (1<<3) 
#define CP_CONTROL_GP_LINK_ENABLE            (1<<4)
#define CP_CONTROL_BREAK_POINT_ENABLE        (1<<5)

#define CP_CLEAR_FIFO_OVERFLOW  (1<<0)
#define CP_CLEAR_FIFO_UNDERFLOW (1<<1)

#define PE_ISR_TOKEN_ENABLE       (1<<0)
#define PE_ISR_FINISH_ENABLE      (1<<1)
#define PE_ISR_TOKEN_ACKNOWLEDGE  (1<<2)
#define PE_ISR_FINISH_ACKNOWLEDGE (1<<3)

// Vertex Index Bits
#define MTXINDX_A_PSN(x)            (((uint32_t)x) << 0) // Position and Normal Matrix index. pairs
#define MTXINDX_A_TEX0(x)           (((uint32_t)x) << 6)
#define MTXINDX_A_TEX1(x)           (((uint32_t)x) << 12)
#define MTXINDX_A_TEX2(x)           (((uint32_t)x) << 18)
#define MTXINDX_A_TEX3(x)           (((uint32_t)x) << 24)

#define MTXINDX_B_TEX4(x)           (((uint32_t)x) << 0)
#define MTXINDX_B_TEX5(x)           (((uint32_t)x) << 6)
#define MTXINDX_B_TEX6(x)           (((uint32_t)x) << 12)
#define MTXINDX_B_TEX7(x)           (((uint32_t)x) << 18)

// Vertex Descriptor Bits
#define VCD_LOW_HAS_POSNRMMTXIDX    (1<<0)
#define VCD_LOW_HAS_TEXCOORDMTXIDX0 (1<<1)
#define VCD_LOW_HAS_TEXCOORDMTXIDX1 (1<<2)
#define VCD_LOW_HAS_TEXCOORDMTXIDX2 (1<<3)
#define VCD_LOW_HAS_TEXCOORDMTXIDX3 (1<<4)
#define VCD_LOW_HAS_TEXCOORDMTXIDX4 (1<<5)
#define VCD_LOW_HAS_TEXCOORDMTXIDX5 (1<<6)
#define VCD_LOW_HAS_TEXCOORDMTXIDX6 (1<<7)
#define VCD_LOW_HAS_TEXCOORDMTXIDX7 (1<<8)
#define VCD_LOW_POS(x)               (((uint32_t)x)<<9)  // The following take gx_vtxattr_data_t
#define VCD_LOW_NRM(x)               (((uint32_t)x)<<11)
#define VCD_LOW_COL0(x)              (((uint32_t)x)<<13)
#define VCD_LOW_COL1(x)              (((uint32_t)x)<<15)

#define VCD_HIGH_TEXCOORD0(x)        (((uint32_t)x)<<0)  // The following take gx_vtxattr_data_t
#define VCD_HIGH_TEXCOORD1(x)        (((uint32_t)x)<<2)
#define VCD_HIGH_TEXCOORD2(x)        (((uint32_t)x)<<4)
#define VCD_HIGH_TEXCOORD3(x)        (((uint32_t)x)<<6)
#define VCD_HIGH_TEXCOORD4(x)        (((uint32_t)x)<<8)
#define VCD_HIGH_TEXCOORD5(x)        (((uint32_t)x)<<10)
#define VCD_HIGH_TEXCOORD6(x)        (((uint32_t)x)<<12)
#define VCD_HIGH_TEXCOORD7(x)        (((uint32_t)x)<<14)

// Vertex Attribute Bits
#define VAT_A_POSCNT(x)              (((uint32_t)x)<<0)  // XY or XYZ
#define VAT_A_POSFMT(x)              (((uint32_t)x)<<1)  // gx_vtxattr_component_format_t
#define VAT_A_POSSHFT(x)             (((uint32_t)x)<<4)  // fractional bits
#define VAT_A_NRMCNT(x)              (((uint32_t)x)<<9)  // N or NBT
#define VAT_A_NRMFMT(x)              (((uint32_t)x)<<10) // gx_vtxattr_component_format_t
#define VAT_A_COL0CNT(x)             (((uint32_t)x)<<13) // RGB/RGBA
#define VAT_A_COL0FMT(x)             (((uint32_t)x)<<14) // gx_vtxattr_component_format_t
#define VAT_A_COL1CNT(x)             (((uint32_t)x)<<17) // RGB/RGBA
#define VAT_A_COL1FMT(x)             (((uint32_t)x)<<18) // gx_vtxattr_component_format_t
#define VAT_A_TEX0CNT(x)             (((uint32_t)x)<<21) // S or ST
#define VAT_A_TEX0FMT(x)             (((uint32_t)x)<<22) // gx_vtxattr_component_format_t
#define VAT_A_TEX0SHFT(x)            (((uint32_t)x)<<25) // fractional bits
#define VAT_A_BYTEDEQUANT            (1<<30)             // Enable shift of u8/s8 components. Should be 1
#define VAT_A_NORMAL_INDEX_3(x)      (((uint32_t)x)<<31) // Enable 3 NBTs. 9 normals.

#define VAT_B_TEX1CNT(x)             (((uint32_t)x)<<0)  // S or ST
#define VAT_B_TEX1FMT(x)             (((uint32_t)x)<<1)  // gx_vtxattr_component_format_t
#define VAT_B_TEX1SHFT(x)            (((uint32_t)x)<<4)  // fractional bits
#define VAT_B_TEX2CNT(x)             (((uint32_t)x)<<9)  // S or ST
#define VAT_B_TEX2FMT(x)             (((uint32_t)x)<<10) // gx_vtxattr_component_format_t
#define VAT_B_TEX2SHFT(x)            (((uint32_t)x)<<13) // fractional bits
#define VAT_B_TEX3CNT(x)             (((uint32_t)x)<<18) // S or ST
#define VAT_B_TEX3FMT(x)             (((uint32_t)x)<<19) // gx_vtxattr_component_format_t
#define VAT_B_TEX3SHFT(x)            (((uint32_t)x)<<22) // fractional bits
#define VAT_B_TEX4CNT(x)             (((uint32_t)x)<<27) // S or ST
#define VAT_B_TEX4FMT(x)             (((uint32_t)x)<<28) // gx_vtxattr_component_format_t
#define VAT_B_VCACHE_ENABLE          (1<<31)             // Enable vertex cache, should be 1

#define VAT_C_TEX4SHFT(x)            (((uint32_t)x)<<0)  // fractional bits
#define VAT_C_TEX5CNT(x)             (((uint32_t)x)<<5)  // S or ST
#define VAT_C_TEX5FMT(x)             (((uint32_t)x)<<6)  // gx_vtxattr_component_format_t
#define VAT_C_TEX5SHFT(x)            (((uint32_t)x)<<9)  // fractional bits
#define VAT_C_TEX6CNT(x)             (((uint32_t)x)<<14) // S or ST
#define VAT_C_TEX6FMT(x)             (((uint32_t)x)<<15) // gx_vtxattr_component_format_t
#define VAT_C_TEX6SHFT(x)            (((uint32_t)x)<<18) // fractional bits
#define VAT_C_TEX7CNT(x)             (((uint32_t)x)<<23) // S or ST
#define VAT_C_TEX7FMT(x)             (((uint32_t)x)<<24) // gx_vtxattr_component_format_t
#define VAT_C_TEX7SHFT(x)            (((uint32_t)x)<<27) // fractional bits

// BP Generation Mode Bits
#define BP_GENMODE_NTEX(x)      ((x & 0xF) << 0)
#define BP_GENMODE_NCOL(x)      ((x & 0x1F) << 4)
#define BP_GENMODE_MS_EN        (1<<9)
#define BP_GENMODE_NTEV(x)      ((x & 0xF) << 10)
#define BP_GENMODE_CULL_MODE(x) ((x & 0b11) << 14)
#define BP_GENMODE_NBMP(x)      ((x & 0b111) << 17)
#define BP_GENMODE_ZFREEZE      (x << 19)

#define BP_ZMODE_ENABLE           (1<<0)
#define BP_ZMODE_FUNC(x)          ((x)<<1)
#define BP_ZMODE_MASK             (1<<4)

#define BP_CMODE0_BLEND_ENABLE    (1<<0)
#define BP_CMODE0_LOGICOP_ENABLE  (1<<1)
#define BP_CMODE0_DITHER_ENABLE   (1<<2)
#define BP_CMODE0_COLOR_MASK      (1<<3)
#define BP_CMODE0_ALPHA_MASK      (1<<4)
#define BP_CMODE0_DFACTOR         (1<<5)
#define BP_CMODE0_SFACTOR         (1<<8)
#define BP_CMODE0_BLENDOP         (1<<11)
#define BP_CMODE0_LOGICOP(x)      ((x) << 12)

#define BP_CMODE1_CONSTANT_ALPHA        (1<<0)
#define BP_CMODE1_CONSTANT_ALPHA_ENABLE (1<<8)
#define BP_CMODE1_YUV_COMPONENT(x)      ((x)<<10) // Not in YAGCD

#define BP_PE_CONTROL_PIXEL_FORMAT(x)  ((x)<<0)
#define BP_PE_CONTROL_Z_FORMAT(x)      ((x)<<3)
#define BP_PE_CONTROL_Z_COMP_LOCK      (1<<6)

#define BP_PE_COPY_EXECUTE_CLAMP(x)       ((x)<<0)
#define BP_PE_COPY_EXECUTE_GAMMA(x)       ((x)<<7)
#define BP_PE_COPY_EXECUTE_Y_SCALE_ENABLE (1<<10) // YAGCD only includes the formula for this bit. So im guessing on its function.
#define BP_PE_COPY_EXECUTE_CLEAR          (1<<11)
#define BP_PE_COPY_EXECUTE_LINE_MODE(x)   ((x)<<12)
#define BP_PE_COPY_EXECUTE_EXECUTE        (1<<14)

#define BP_PE_DONE_END_OF_LIST  (1<<1)

// 2 MB Embedded Frame Buffer
#define EFB                       ((volatile uint32_t*)0xC8000000)

// Used to operate GX and update things as needed

#define GX_DIRTY_VAT_FMT_NEEDS_UPDATE(x)         (1<<x) // Update one of the vertex attribute table entrys.
#define GX_DIRTY_VCD_NEEDS_UPDATE                (1<<8) // Updates the VCD at the next draw call. Updates XF statistics.
#define GX_DIRTY_XF_COLORS_NEEDS_UPDATE          (1<<9) // Update the color count and control registers of the XF
#define GX_DIRTY_XF_TEXCOORD_NEEDS_UPDATE        (1<<10) // Updates the texture coord count and control registers of the XF
#define GX_DIRTY_MATRIX_INDEX_NEEDS_UPDATE       (1<<11) // Update position/normal/texture matrix indexes
#define GX_DIRTY_BP_GENMODE_NEEDS_UPDATE         (1<<12) // Update the BP generation mode register

#define BIT(v, bit, set) ((set) ? (v | bit) : (v & ~bit))

static struct {
    // We save the register states then play them out to hardware when their needed.
    // This is similar to what libogc does. The point being to minimize pipeline overhead
    // and keep compatibility with libogc programs.
    uint32_t dirty;
    uint32_t matrix_index_a;
    uint32_t matrix_index_b;
    uint32_t vcd_low;
    uint32_t vcd_high;
    uint32_t vat_tables[3][8]; // 3 sets, 8 formats.
    uint32_t normals;   // Normal settings, 0: none, 1: 1 normal, 2: NBT (3)
    uint32_t genmode; // Stores the color and texcoord settings to be passed from XF to BP/TEV
    uint32_t xf_color_settings[4]; // Updated with genmode.

    uint32_t bp_efb_top_left; // Frame buffer copy settings
    uint32_t bp_efb_width_height;
    uint32_t bp_xfb_width_stride;
    uint32_t z_mode;   // Saved to be restored when needed
    uint32_t c_mode_0; // Saved to be restored when needed
    uint32_t c_mode_1; // Saved to be restored when needed
    uint32_t pe_control; // Saved to be restored when needed
    uint32_t pe_copy_execute;
} gx_state;

static StaticSemaphore_t semaphores_static;
static SemaphoreHandle_t gx_pe_finish_semaphore; 

// Called when the pixel engine finishes its work.
static void gx_irq_pe_finish(exception_irq_type_t irq) {
    // Acknowledge.
    PE_ISR |= PE_ISR_FINISH_ACKNOWLEDGE;

    // Alert waiting task of draw finish
    xSemaphoreGiveFromISR(gx_pe_finish_semaphore, &exception_isr_context_switch_needed);
}

// This updates the VCD.
// It also updates the status register GX_XF_REGISTER_VERTEX_STATS
// This is different from the COLOR_COUNT register, who is how many
// Colors to supply to the TEV/BP. This instead is how many the XF expects
// to be supplied. Pipelining uwu
static void gx_flush_vcd() {
    // Update registers in VCD
    GX_WPAR_CP_LOAD(GX_CP_REGISTER_VCD_LOW, gx_state.vcd_low);
    GX_WPAR_CP_LOAD(GX_CP_REGISTER_VCD_HIGH, gx_state.vcd_high);

    // XF Statistics
    uint32_t texcoords=0, colors=0;

    // Texcoords
    if(gx_state.vcd_high & VCD_HIGH_TEXCOORD0(3)) texcoords++;
    if(gx_state.vcd_high & VCD_HIGH_TEXCOORD1(3)) texcoords++;
    if(gx_state.vcd_high & VCD_HIGH_TEXCOORD2(3)) texcoords++;
    if(gx_state.vcd_high & VCD_HIGH_TEXCOORD3(3)) texcoords++;
    if(gx_state.vcd_high & VCD_HIGH_TEXCOORD4(3)) texcoords++;
    if(gx_state.vcd_high & VCD_HIGH_TEXCOORD5(3)) texcoords++;
    if(gx_state.vcd_high & VCD_HIGH_TEXCOORD6(3)) texcoords++;
    if(gx_state.vcd_high & VCD_HIGH_TEXCOORD7(3)) texcoords++;

    // Colors
    if(gx_state.vcd_low & VCD_LOW_COL0(3)) colors++;
    if(gx_state.vcd_low & VCD_LOW_COL1(3)) colors++;

    GX_WPAR_XF_LOAD(GX_XF_REGISTER_VERTEX_STATS, 1);
    GX_WPAR_U32 = (texcoords << 2) | (gx_state.normals << 2) | colors;
}

static void gx_flush_xf_color_settings() {
    // Extract color count from bp settings
    GX_WPAR_XF_LOAD(GX_XF_REGISTER_COLOR_COUNT, 1);
    GX_WPAR_U32 = (gx_state.genmode >> 4) & 0b11;

    // For now load default color mode. no lighting
    GX_WPAR_XF_LOAD(GX_XF_REGISTER_COLOR_CONTROL, 4);
    GX_WPAR_U32 = gx_state.xf_color_settings[0];
    GX_WPAR_U32 = gx_state.xf_color_settings[1];
    GX_WPAR_U32 = gx_state.xf_color_settings[2];
    GX_WPAR_U32 = gx_state.xf_color_settings[3];
}

static void gx_flush_texcoord_settings() {
    GX_WPAR_XF_LOAD(GX_XF_REGISTER_TEXCOORD_COUNT, 1);
    GX_WPAR_U32 = (gx_state.genmode >> 0) & 0xF;
}

static void gx_flush_matrix_indexes() {
    GX_WPAR_CP_LOAD(GX_CP_REGISTERS_MTXIDX_A, gx_state.matrix_index_a);
    GX_WPAR_CP_LOAD(GX_CP_REGISTERS_MTXIDX_B, gx_state.matrix_index_b);

    /// TODO: Can you load multiple XF registers in 1 command, or is that just
    // for XF Memory?
    GX_WPAR_XF_LOAD(GX_XF_REGISTERS_MTXIDX_A, 1);
    GX_WPAR_U32 = gx_state.matrix_index_a;
    GX_WPAR_XF_LOAD(GX_XF_REGISTERS_MTXIDX_B, 1);
    GX_WPAR_U32 = gx_state.matrix_index_b;
}

static void gx_flush_bp_genmode() {
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_GENMODE | gx_state.genmode);
}

static void gx_flush_vat_format(uint32_t vat_offset) {
    GX_WPAR_CP_LOAD(GX_CP_REGISTER_VAT_A + vat_offset, gx_state.vat_tables[0][vat_offset / 4]);
    GX_WPAR_CP_LOAD(GX_CP_REGISTER_VAT_B + vat_offset, gx_state.vat_tables[1][vat_offset / 4]);
    GX_WPAR_CP_LOAD(GX_CP_REGISTER_VAT_C + vat_offset, gx_state.vat_tables[2][vat_offset / 4]);
}

static void gx_flush_state() {
    // Early exit.
    if(gx_state.dirty == 0)
        return;

    // 8 VAT Table Formats.
    // Go through each bit, if each format needs updating
    uint32_t j = 0;
    for(uint32_t i = gx_state.dirty & 0xFF; i != 0; i >>= 1) {
        if(i & 1) {
            gx_flush_vat_format(j);
        }

        j += 4;
    }

    if(gx_state.dirty & GX_DIRTY_VCD_NEEDS_UPDATE) {
        // Update VCD and XF enable/disable pipeline for vertex description
        gx_flush_vcd();
    }
    if(gx_state.dirty & GX_DIRTY_XF_COLORS_NEEDS_UPDATE) {
        // Update the color count to BP and settings
        gx_flush_xf_color_settings();
    }
    if(gx_state.dirty & GX_DIRTY_XF_TEXCOORD_NEEDS_UPDATE) {
        // Update the color count to BP and settings
        gx_flush_texcoord_settings();
    }
    if(gx_state.dirty & GX_DIRTY_MATRIX_INDEX_NEEDS_UPDATE) {
        // Updates them in both the CP and XF.
        gx_flush_matrix_indexes();
    }
    if(gx_state.dirty & GX_DIRTY_BP_GENMODE_NEEDS_UPDATE) {
        gx_flush_bp_genmode();
    }

    gx_state.dirty = 0;
}

void gx_initialize(const gx_fifo_t* fifo, const video_profile_t* video_profile) {
    // Disconnect pipelines while we work on them
    CP_CONTROL = 0;
    
    // Clear State registers to zero
    // This should be set later on, just need to make sure!
    memset(&gx_state, 0, sizeof(gx_state));

    // Set the current hardware fifo
    // Will be needed to configure later on
    gx_fifo_set(fifo);

    // It will read commands and pass them on to the pipeline.
    CP_CONTROL = CP_CONTROL_FIFO_READ | CP_CONTROL_GP_LINK_ENABLE;

    // GX FIFO seems to ignore some of the first commands
    // May be an attaching/detaching issue. This helps though.
    // Could causes crashes on boot though if we are flushing
    // bogas commands.
    gx_flush();

    gx_initialize_state();
    gx_initialize_video(video_profile);

    // Flush it out for now
    gx_flush_state();

    // Flush again so our changes reach the pipeline
    gx_flush();

    // Create semaphore for pe finish (draw done, begin copy)
    gx_pe_finish_semaphore = xSemaphoreCreateCountingStatic(4, 0, &semaphores_static);

    // Install interrupt handler
    exceptions_install_irq(gx_irq_pe_finish, EXCEPTION_IRQ_TYPE_PE_FINISH);

    // Enable interrupts
    PE_ISR = PE_ISR_FINISH_ENABLE;
}

void gx_initialize_state() {
    // All vertex descriptions off
    gx_vtxdesc_clear();

    // Empty vertex formats
    for(int i = 0; i < 8; i++)
        gx_vtxfmtattr_clear(i);
    
    // Clear matrix indices
    gx_state.matrix_index_a = 0;
    gx_state.matrix_index_b = 0;
    gx_state.dirty |= GX_DIRTY_MATRIX_INDEX_NEEDS_UPDATE;

    // Turn off all gens
    gx_state.genmode = 0;
    gx_set_color_channels(0);
    gx_set_texcoord_channels(0);

    // Default XF Lighting
    gx_light_t empty;
    memset(&empty, 0, sizeof(empty));
    gx_configure_color_channel(GX_COLOR_CHANNEL_COLOR0, 0, false, true, true, GX_DIFFUSE_MODE_NONE, GX_ATTENUATION_MODE_NONE);
    gx_configure_color_channel(GX_COLOR_CHANNEL_COLOR1, 0, false, true, true, GX_DIFFUSE_MODE_NONE, GX_ATTENUATION_MODE_NONE);
    gx_configure_color_channel(GX_COLOR_CHANNEL_ALPHA0, 0, false, true, true, GX_DIFFUSE_MODE_NONE, GX_ATTENUATION_MODE_NONE);
    gx_configure_color_channel(GX_COLOR_CHANNEL_ALPHA1, 0, false, true, true, GX_DIFFUSE_MODE_NONE, GX_ATTENUATION_MODE_NONE);
    gx_flash_light(GX_LIGHT_ID_0, &empty);
    gx_flash_light(GX_LIGHT_ID_1, &empty);
    gx_flash_light(GX_LIGHT_ID_2, &empty);
    gx_flash_light(GX_LIGHT_ID_3, &empty);
    gx_flash_light(GX_LIGHT_ID_4, &empty);
    gx_flash_light(GX_LIGHT_ID_5, &empty);
    gx_flash_light(GX_LIGHT_ID_6, &empty);
    gx_flash_light(GX_LIGHT_ID_7, &empty);
    gx_flash_xf_color(GX_XF_COLOR_AMBIENT_0, 0, 0, 0, 255);
    gx_flash_xf_color(GX_XF_COLOR_AMBIENT_1, 0, 0, 0, 255);
    gx_flash_xf_color(GX_XF_COLOR_MATERIAL_0, 0, 0, 0, 255);
    gx_flash_xf_color(GX_XF_COLOR_MATERIAL_1, 0, 0, 0, 255);

    // Default PE setup
    gx_set_z_mode(true, GX_COMPARE_LESS_EQUAL, true); // Z filtering? YES
    gx_set_color_update(true, true);
    gx_enable_z_precheck(true);
    gx_set_pixel_format(GX_PIXEL_FORMAT_RGB8_Z24, GX_Z_FORMAT_LINEAR);
    gx_set_clamp_mode(GX_CLAMP_MODE_TOP_BOTTOM);
    gx_set_gamma(GX_GAMMA_1_0);
    gx_set_line_mode(GX_LINE_MODE_PROGRESSIVE);
    gx_set_clear_color(0, 0, 0, 0);
    gx_set_clear_z(0xFFFFFF);
}

void gx_initialize_video(const video_profile_t* video_profile) {
    gx_flash_viewport(0.0f, 0.0f, video_profile->width, video_profile->efb_height, 0.0f, 1.0f, true);
    gx_set_copy_y_scale((float)video_profile->xfb_height / (float)video_profile->efb_height);
    gx_set_scissor_rectangle(0, 0, video_profile->width, video_profile->efb_height);
    gx_set_copy_window(0, 0, video_profile->width, video_profile->efb_height, video_profile->width);
    gx_set_copy_filter(video_profile->copy_pattern, video_profile->copy_filer);
}

void gx_fifo_create(gx_fifo_t* fifo, void* fifo_buffer, uint32_t fifo_buffer_size) {
    // Clear that thing
    memset(fifo_buffer, 0, fifo_buffer_size);

    // Really clear that thing
    system_flush_dcache(fifo_buffer, fifo_buffer_size);
    
    fifo->base_address = SYSTEM_MEM_PHYSICAL(fifo_buffer);
    fifo->end_address = fifo->base_address + fifo_buffer_size - 4;
    fifo->high_watermark = 0;
    fifo->low_watermark = 0;
    fifo->distance = 0;
    fifo->write_head = fifo->base_address;
    fifo->read_head = fifo->base_address;
    fifo->breakpoint = 0;
}

void gx_fifo_set(const gx_fifo_t* fifo) {
    // Set FIFO Config.
    CP_FIFO_BASE_LOW = fifo->base_address & 0xFFFF;
    CP_FIFO_BASE_HIGH = fifo->base_address >> 16;
    CP_FIFO_END_LOW = fifo->end_address & 0xFFFF;
    CP_FIFO_END_HIGH = fifo->end_address >> 16;
    CP_HIGH_WATERMARK_LOW = fifo->high_watermark & 0xFFFF;
    CP_HIGH_WATERMARK_HIGH = fifo->high_watermark >> 16;
    CP_LOW_WATERMARK_LOW = fifo->low_watermark & 0xFFFF;
    CP_LOW_WATERMARK_HIGH = fifo->low_watermark >> 16;
    CP_FIFO_DISTANCE_LOW = fifo->distance & 0xFFFF;
    CP_FIFO_DISTANCE_HIGH = fifo->distance >> 16;
    CP_FIFO_WRITE_HEAD_LOW = fifo->write_head & 0xFFFF;
    CP_FIFO_WRITE_HEAD_HIGH = fifo->write_head >> 16;
    CP_FIFO_READ_HEAD_LOW = fifo->read_head & 0xFFFF;
    CP_FIFO_READ_HEAD_HIGH = fifo->read_head >> 16;
    CP_FIFO_BREAKPOINT_LOW = fifo->base_address & 0xFFFF;
    CP_FIFO_BREAKPOINT_HIGH = fifo->base_address >> 16;

    // Set it in the processor interface too
    PI_FIFO_BASE = fifo->base_address;
    PI_FIFO_END = fifo->end_address;
    PI_FIFO_WRITE_HEAD = fifo->write_head;
}

void gx_fifo_get(gx_fifo_t* fifo) {
    // Get FIFO Config.
    fifo->base_address = (CP_FIFO_BASE_HIGH << 16) | CP_FIFO_BASE_LOW;
    fifo->end_address = (CP_FIFO_END_HIGH << 16) | CP_FIFO_END_LOW;
    fifo->high_watermark = (CP_HIGH_WATERMARK_HIGH << 16) | CP_HIGH_WATERMARK_LOW;
    fifo->low_watermark = (CP_LOW_WATERMARK_HIGH << 16) | CP_LOW_WATERMARK_LOW;
    fifo->distance = (CP_FIFO_DISTANCE_HIGH << 16) | CP_FIFO_DISTANCE_LOW;
    fifo->write_head = (CP_FIFO_WRITE_HEAD_HIGH << 16) | CP_FIFO_WRITE_HEAD_LOW;
    fifo->read_head = (CP_FIFO_READ_HEAD_HIGH << 16) | CP_FIFO_READ_HEAD_LOW;
    fifo->breakpoint = (CP_FIFO_BREAKPOINT_HIGH << 16) | CP_FIFO_BREAKPOINT_LOW;
}

void gx_flush() {
    // Write 32 NO OP commands.
    // This is the size of 1 WPAR transaction
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
}

uint32_t gx_efb_peak(uint32_t x, uint32_t y) {
    // Decode pixel address
    uint32_t pixel_offset = (y & 0x3FF) * 1024 + (x & 0x3FF);

    uint32_t color = EFB[pixel_offset];

    // We sort the colors RGBA, it sorts them ARGB, so lets fix that
    return (color << 8) | (color >> 24);
}

void gx_vtxdesc_clear() {
    gx_state.vcd_low = 0;
    gx_state.vcd_high = 0;
    gx_state.normals = 0;
    gx_state.dirty |= GX_DIRTY_VCD_NEEDS_UPDATE;
}

void gx_vtxdesc_set(gx_vtxdesc_t desc, gx_vtxattr_data_t type) {
    // Only mark dirty if these actually change.
    uint16_t new_vcd_low = gx_state.vcd_low;
    uint16_t new_vcd_high = gx_state.vcd_high;

    switch(desc) {
        case GX_VTXDESC_POSNORM_INDEX:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_POSNRMMTXIDX, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_TEXCOORDMTX0:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_TEXCOORDMTXIDX0, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_TEXCOORDMTX1:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_TEXCOORDMTXIDX1, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_TEXCOORDMTX2:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_TEXCOORDMTXIDX2, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_TEXCOORDMTX3:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_TEXCOORDMTXIDX3, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_TEXCOORDMTX4:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_TEXCOORDMTXIDX4, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_TEXCOORDMTX5:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_TEXCOORDMTXIDX5, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_TEXCOORDMTX6:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_TEXCOORDMTXIDX6, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_TEXCOORDMTX7:
            new_vcd_low = BIT(new_vcd_low, VCD_LOW_HAS_TEXCOORDMTXIDX7, type == GX_VTXATTR_DATA_DIRECT);
            break;
        case GX_VTXDESC_POSITION:
            new_vcd_low = (new_vcd_low & ~VCD_LOW_POS(0b11)) | VCD_LOW_POS(type);
            break;
        case GX_VTXDESC_NORMAL:
            new_vcd_low = (new_vcd_low & ~VCD_LOW_NRM(0b11)) | VCD_LOW_NRM(type);
            gx_state.normals = 1;
            break;
        case GX_VTXDESC_NORMAL_NBT:
            new_vcd_low = (new_vcd_low & ~VCD_LOW_NRM(0b11)) | VCD_LOW_NRM(type);
            gx_state.normals = 2;
            break;
        case GX_VTXDESC_COLOR0:
            new_vcd_low = (new_vcd_low & ~VCD_LOW_COL0(0b11)) | VCD_LOW_COL0(type);
            break;
        case GX_VTXDESC_COLOR1:
            new_vcd_low = (new_vcd_low & ~VCD_LOW_COL1(0b11)) | VCD_LOW_COL1(type);
            break;
        case GX_VTXDESC_TEXCOORD0:
            new_vcd_high = (new_vcd_high & ~VCD_HIGH_TEXCOORD0(0b11)) | VCD_HIGH_TEXCOORD0(type);
            break;
        case GX_VTXDESC_TEXCOORD1:
            new_vcd_high = (new_vcd_high & ~VCD_HIGH_TEXCOORD1(0b11)) | VCD_HIGH_TEXCOORD1(type);
            break;
        case GX_VTXDESC_TEXCOORD2:
            new_vcd_high = (new_vcd_high & ~VCD_HIGH_TEXCOORD2(0b11)) | VCD_HIGH_TEXCOORD2(type);
            break;
        case GX_VTXDESC_TEXCOORD3:
            new_vcd_high = (new_vcd_high & ~VCD_HIGH_TEXCOORD3(0b11)) | VCD_HIGH_TEXCOORD3(type);
            break;
        case GX_VTXDESC_TEXCOORD4:
            new_vcd_high = (new_vcd_high & ~VCD_HIGH_TEXCOORD4(0b11)) | VCD_HIGH_TEXCOORD4(type);
            break;
        case GX_VTXDESC_TEXCOORD5:
            new_vcd_high = (new_vcd_high & ~VCD_HIGH_TEXCOORD5(0b11)) | VCD_HIGH_TEXCOORD5(type);
            break;
        case GX_VTXDESC_TEXCOORD6:
            new_vcd_high = (new_vcd_high & ~VCD_HIGH_TEXCOORD6(0b11)) | VCD_HIGH_TEXCOORD6(type);
            break;
        case GX_VTXDESC_TEXCOORD7:
            new_vcd_high = (new_vcd_high & ~VCD_HIGH_TEXCOORD7(0b11)) | VCD_HIGH_TEXCOORD7(type);
            break;
        default:
            LOG_ERROR(TAB, "Invalid vertex descriptor type %d!", desc);
            break;
    }

    if(new_vcd_low != gx_state.vcd_low || new_vcd_high != gx_state.vcd_high) {
        gx_state.vcd_low = new_vcd_low;
        gx_state.vcd_high = new_vcd_high;
        gx_state.dirty |= GX_DIRTY_VCD_NEEDS_UPDATE;
    }
}

void gx_vtxfmtattr_clear(uint8_t attribute_index) {
    gx_state.vat_tables[0][attribute_index] = VAT_A_BYTEDEQUANT;
    gx_state.vat_tables[1][attribute_index] = VAT_B_VCACHE_ENABLE;
    gx_state.vat_tables[2][attribute_index] = 0;
    gx_state.dirty |= GX_DIRTY_VAT_FMT_NEEDS_UPDATE(attribute_index);
}

void gx_vtxfmtattr_set(uint8_t attribute_index, gx_vtxdesc_t attribute, gx_vtxattr_component_t component, gx_vtxattr_component_format_t fmt, uint8_t fraction) {
    // Only mark dirty if we changed anything
    uint32_t vat_a = gx_state.vat_tables[0][attribute_index];
    uint32_t vat_b = gx_state.vat_tables[1][attribute_index];
    uint32_t vat_c = gx_state.vat_tables[2][attribute_index];

    switch(attribute) {
        case GX_VTXDESC_POSITION:
            vat_a &= ~(VAT_A_POSCNT(0b1) | VAT_A_POSFMT(0b111) | VAT_A_POSSHFT(0b11111));
            vat_a |= VAT_A_POSCNT(component) | VAT_A_POSFMT(fmt) | VAT_A_POSSHFT(fraction);
            break;
        case GX_VTXDESC_NORMAL:
            vat_a &= ~(VAT_A_NRMCNT(0b1) | VAT_A_NRMFMT(0b111) | VAT_A_NORMAL_INDEX_3(0b1));
            vat_a |= VAT_A_NRMCNT(component & 0b1) | VAT_A_NRMFMT(fmt) | VAT_A_NORMAL_INDEX_3((component >> 1) & 1);
            break;
        case GX_VTXDESC_COLOR0:
            vat_a &= ~(VAT_A_COL0CNT(0b1) | VAT_A_COL0FMT(0b111));
            vat_a |= VAT_A_COL0CNT(component) | VAT_A_COL0FMT(fmt);
            break;
        case GX_VTXDESC_COLOR1:
            vat_a &= ~(VAT_A_COL1CNT(0b1) | VAT_A_COL1FMT(0b111));
            vat_a |= VAT_A_COL1CNT(component) | VAT_A_COL1FMT(fmt);
            break;
        case GX_VTXDESC_TEXCOORD0:
            vat_a &= ~(VAT_A_TEX0CNT(0b1) | VAT_A_TEX0FMT(0b111) | VAT_A_TEX0SHFT(0b11111));
            vat_a |= VAT_A_TEX0CNT(component) | VAT_A_TEX0FMT(fmt) | VAT_A_TEX0SHFT(fraction);
            break;
        case GX_VTXDESC_TEXCOORD1:
            vat_b &= ~(VAT_B_TEX1CNT(0b1) | VAT_B_TEX1FMT(0b111) | VAT_B_TEX1SHFT(0b11111));
            vat_b |= VAT_B_TEX1CNT(component) | VAT_B_TEX1FMT(fmt) | VAT_B_TEX1SHFT(fraction);
            break;
        case GX_VTXDESC_TEXCOORD2:
            vat_b &= ~(VAT_B_TEX2CNT(0b1) | VAT_B_TEX2FMT(0b111) | VAT_B_TEX2SHFT(0b11111));
            vat_b |= VAT_B_TEX2CNT(component) | VAT_B_TEX2FMT(fmt) | VAT_B_TEX2SHFT(fraction);
            break;
        case GX_VTXDESC_TEXCOORD3:
            vat_b &= ~(VAT_B_TEX3CNT(0b1) | VAT_B_TEX3FMT(0b111) | VAT_B_TEX3SHFT(0b11111));
            vat_b |= VAT_B_TEX3CNT(component) | VAT_B_TEX3FMT(fmt) | VAT_B_TEX3SHFT(fraction);
            break;
        case GX_VTXDESC_TEXCOORD4:
            vat_b &= ~(VAT_B_TEX4CNT(0b1) | VAT_B_TEX4FMT(0b111));
            vat_b |= VAT_B_TEX4CNT(component) | VAT_B_TEX4FMT(fmt);
            vat_c &= ~VAT_C_TEX4SHFT(0b11111);
            vat_c |= VAT_C_TEX4SHFT(fraction);
            break;
        case GX_VTXDESC_TEXCOORD5:
            vat_c &= ~(VAT_C_TEX5CNT(0b1) | VAT_C_TEX5FMT(0b111) | VAT_C_TEX5SHFT(0b11111));
            vat_c |= VAT_C_TEX5CNT(component) | VAT_C_TEX5FMT(fmt) | VAT_C_TEX5SHFT(fraction);
            break;
        case GX_VTXDESC_TEXCOORD6:
            vat_c &= ~(VAT_C_TEX6CNT(0b1) | VAT_C_TEX6FMT(0b111) | VAT_C_TEX6SHFT(0b11111));
            vat_c |= VAT_C_TEX6CNT(component) | VAT_C_TEX6FMT(fmt) | VAT_C_TEX6SHFT(fraction);
            break;
        case GX_VTXDESC_TEXCOORD7:
            vat_c &= ~(VAT_C_TEX7CNT(0b1) | VAT_C_TEX7FMT(0b111) | VAT_C_TEX7SHFT(0b11111));
            vat_c |= VAT_C_TEX7CNT(component) | VAT_C_TEX7FMT(fmt) | VAT_C_TEX7SHFT(fraction);
            break;
        default:
            LOG_ERROR(TAB, "Invalid vertex attribute format type %d!", attribute);
            break;
    }

    if(vat_a != gx_state.vat_tables[0][attribute_index] || vat_b != gx_state.vat_tables[1][attribute_index] || vat_c != gx_state.vat_tables[2][attribute_index]) {
        gx_state.vat_tables[0][attribute_index] = vat_a;
        gx_state.vat_tables[1][attribute_index] = vat_b;
        gx_state.vat_tables[2][attribute_index] = vat_c;
        gx_state.dirty |= GX_DIRTY_VAT_FMT_NEEDS_UPDATE(attribute_index);
    }
}

void gx_set_texcoord_channels(uint32_t count) {
    gx_state.genmode = (gx_state.genmode & ~BP_GENMODE_NTEX(0xF)) | BP_GENMODE_NTEV(count);
    gx_state.dirty |= GX_DIRTY_BP_GENMODE_NEEDS_UPDATE | GX_DIRTY_XF_TEXCOORD_NEEDS_UPDATE;
}

void gx_begin(gx_primitive_t primitive, uint8_t attribute, uint16_t count) {
    // We flush the GPU state, like vertex descriptions up until the draw call.
    // So that the user may modify them how they like without bogging the pipeline down
    // with redundant calls, then we flush it all at once
    gx_flush_state();

    GX_WPAR_U8 = GX_WPAR_OPCODE_PRIMITIVE(primitive, attribute);
    GX_WPAR_U16 = count;
}

void gx_draw_done() {
    // Discard supurfiouse events
    while (xSemaphoreTake(gx_pe_finish_semaphore, 0) == pdTRUE);

    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_PE_DONE | BP_PE_DONE_END_OF_LIST);
    gx_flush();

    // Wait for it to signal draw done
    xSemaphoreTake(gx_pe_finish_semaphore, portMAX_DELAY);
}

void gx_set_clear_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_PE_COPY_CLEAR_AR | (a << 8) | r);
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_PE_COPY_CLEAR_GB | (g << 8) | b);
}

void gx_set_clear_z(uint32_t z) {
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_PE_COPY_CLEAR_Z | (z & 0xFFFFFF));
}

void gx_set_copy_y_scale(float y_scale) {
    uint32_t scale = (uint32_t)(256.0f / y_scale); // From YAGD. Fun formula
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_Y_SCALE | (scale & 0x1FF));

    // If we have a y scale, other than 1, then set the enable bit
    if(y_scale > 1.0f) {
        gx_state.pe_copy_execute |= BP_PE_COPY_EXECUTE_Y_SCALE_ENABLE;
    } else {
        gx_state.pe_copy_execute &= ~BP_PE_COPY_EXECUTE_Y_SCALE_ENABLE;
    }
}

void gx_set_scissor_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    uint32_t left = x + 342;
    uint32_t top = y + 342;
    uint32_t right = left + width - 1;
    uint32_t bottom = top + height - 1;

    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_SCISSOR_TL | (top & 0x7FF) | ((left & 0x7FF) << 12));
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_SCISSOR_BR | (bottom & 0x7FF) | ((right & 0x7FF) << 12));
}

void gx_set_scissor_offset(int32_t x, int32_t y) {
    int32_t xo = (x + 342) >> 1;
    int32_t yo = (y + 342) >> 1;
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_SCISSOR_OFFSET | (xo & 0x3FF) | ((yo & 0x3FF) << 10));
}

void gx_set_copy_window(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t xfb_width) {
    gx_state.bp_efb_top_left = (y << 10) | y;
    gx_state.bp_efb_width_height = ((height-1) << 10) | (width-1);
    gx_state.bp_xfb_width_stride = width / 16;
}

void gx_set_clamp_mode(gx_clamp_mode_t mode) {
    gx_state.pe_copy_execute = (gx_state.pe_copy_execute & ~BP_PE_COPY_EXECUTE_CLAMP(0b11)) | BP_PE_COPY_EXECUTE_CLAMP(mode);
}

void gx_set_gamma(gx_gamma_t gamma) {
    gx_state.pe_copy_execute = (gx_state.pe_copy_execute & ~BP_PE_COPY_EXECUTE_GAMMA(0b11)) | BP_PE_COPY_EXECUTE_GAMMA(gamma);
}

void gx_set_line_mode(gx_line_mode_t line_mode) {
    gx_state.pe_copy_execute = (gx_state.pe_copy_execute & ~BP_PE_COPY_EXECUTE_LINE_MODE(0b11)) | BP_PE_COPY_EXECUTE_LINE_MODE(line_mode);
}

void gx_set_copy_filter(const uint8_t pattern[12][2], const uint8_t filter[7]) {
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_COPY_FILTER_POS_A | (pattern[0][0] << 0)
                                                      | (pattern[0][1] << 4)
                                                      | (pattern[1][0] << 8)
                                                      | (pattern[1][1] << 12)
                                                      | (pattern[2][0] << 16)
                                                      | (pattern[2][1] << 20));
    
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_COPY_FILTER_POS_B | (pattern[3][0] << 0)
                                                      | (pattern[3][1] << 4)
                                                      | (pattern[4][0] << 8)
                                                      | (pattern[4][1] << 12)
                                                      | (pattern[5][0] << 16)
                                                      | (pattern[5][1] << 20));
    
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_COPY_FILTER_POS_C | (pattern[6][0] << 0)
                                                      | (pattern[6][1] << 4)
                                                      | (pattern[7][0] << 8)
                                                      | (pattern[7][1] << 12)
                                                      | (pattern[8][0] << 16)
                                                      | (pattern[8][1] << 20));
    
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_COPY_FILTER_POS_D | (pattern[9][0] << 0)
                                                      | (pattern[9][1] << 4)
                                                      | (pattern[10][0] << 8)
                                                      | (pattern[10][1] << 12)
                                                      | (pattern[11][0] << 16)
                                                      | (pattern[11][1] << 20));
    
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_COPY_FILTER_COEFF_A | (filter[0] << 0)
                                                        | (filter[1] << 6)
                                                        | (filter[2] << 12)
                                                        | (filter[3] << 18));
    
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_COPY_FILTER_COEFF_B | (filter[4] << 0)
                                                        | (filter[5] << 6)
                                                        | (filter[6] << 12));
}

void gx_set_z_mode(bool enable_compare, gx_compare_t compare, bool enable_update) {
    gx_state.z_mode = GX_BP_REGISTERS_Z_MODE |
                      (enable_compare ? (1 << 0) : 0) |
                      ((uint32_t)compare << 1) |
                      (enable_update ? (1 << 4) : 0);
    GX_WPAR_BP_LOAD(gx_state.z_mode);
}

void gx_set_color_update(bool update_color, bool update_alpha) {
    if(update_color)
        gx_state.c_mode_0 |= BP_CMODE0_COLOR_MASK;
    else 
        gx_state.c_mode_0 &= ~BP_CMODE0_COLOR_MASK;
    
    if(update_alpha)
        gx_state.c_mode_0 |= BP_CMODE0_ALPHA_MASK;
    else 
        gx_state.c_mode_0 &= ~BP_CMODE0_ALPHA_MASK;
}

void gx_enable_z_precheck(bool enable) {
    if(enable)
        gx_state.pe_control |= BP_PE_CONTROL_Z_COMP_LOCK;
    else
        gx_state.pe_control &= ~BP_PE_CONTROL_Z_COMP_LOCK;
}

void gx_set_pixel_format(gx_pixel_format_t pixels_format, gx_z_format_t z_format) {
    // On hardware the Y8, U8, and V8 are all the same mode.
    // Had to learn this from libogc. not on YAGCD.
    uint8_t hardware_formats[8] = {1, 2, 3, 4, 4, 4, 5};
    int hardware_format = hardware_formats[pixels_format];

    // Update the PE control register
    gx_state.pe_control = (gx_state.pe_control & ~(BP_PE_CONTROL_PIXEL_FORMAT(0b111) | BP_PE_CONTROL_Z_FORMAT(0b11)))
                        | BP_PE_CONTROL_PIXEL_FORMAT(hardware_format) | BP_PE_CONTROL_Z_FORMAT(z_format);
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_PE_CONTROL | gx_state.pe_control);

    // Update ms enable
    if(pixels_format == GX_PIXEL_FORMAT_RGB565_Z16)
        gx_state.genmode |= BP_GENMODE_MS_EN;
    else
        gx_state.genmode &= ~BP_GENMODE_MS_EN;
    gx_state.dirty |= GX_DIRTY_BP_GENMODE_NEEDS_UPDATE;

    // Write the actual pixel formats for Y8, U8 and V8 to cmode1
    if(hardware_format == 4) {
        int cmode_format = hardware_format - 4;
        gx_state.c_mode_1 = (gx_state.c_mode_1 & ~BP_CMODE1_YUV_COMPONENT(0b11)) | BP_CMODE1_YUV_COMPONENT(cmode_format);
        GX_WPAR_BP_LOAD(GX_BP_REGISTERS_CMODE_1 | gx_state.c_mode_1);
    }
}

void gx_copy_framebuffer(framebuffer_t* framebuffer, bool clear) {
    if(clear) {
        // Go ahead and make it always update the Z value when clearing
        GX_WPAR_BP_LOAD(GX_BP_REGISTERS_Z_MODE | (gx_state.z_mode | BP_ZMODE_ENABLE | BP_ZMODE_FUNC(GX_COMPARE_ALWAYS)));

        // Disable LOGICOP and BLEND for clear
        GX_WPAR_BP_LOAD(GX_BP_REGISTERS_CMODE_0 | (gx_state.c_mode_0 & ~(BP_CMODE0_BLEND_ENABLE | BP_CMODE0_LOGICOP_ENABLE)));
    }

    // If we have early z compare on, and were clearing or its just an all Z framebuffer,
    // Then we will need to disable early compare before clearing.
    // (someone should tell me why this behavior is)
    bool pe_control_dirty = false;
    if((gx_state.pe_control & BP_PE_CONTROL_Z_COMP_LOCK) && (clear || ((gx_state.pe_control & 0b111) == GX_PIXEL_FORMAT_Z24))) {
        GX_WPAR_BP_LOAD(GX_BP_REGISTERS_PE_CONTROL | (gx_state.pe_control & ~BP_PE_CONTROL_Z_COMP_LOCK));
        pe_control_dirty = true;
    }

    // Set copy bounds
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_EFB_SOURCE_TOP_LEFT | gx_state.bp_efb_top_left);
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_EFB_SOURCE_WIDTH_HEIGHT | gx_state.bp_efb_width_height);
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_EFB_DESTINATION_WIDTH | gx_state.bp_xfb_width_stride);

    // Set the address of our magical frame buffer
    uint32_t physical_addresss = SYSTEM_MEM_PHYSICAL(framebuffer) >> 5; // 32 byte algined. pulls off those 5 lsb
    GX_WPAR_BP_LOAD(GX_BP_REGISTERS_XFB_TARGET_ADDRESS | (physical_addresss & 0x00FFFFFF));

    // Copy it!
    uint32_t clear_command = GX_BP_REGISTERS_PE_COPY_EXECUTE | BP_PE_COPY_EXECUTE_EXECUTE;
    if(clear)
        clear_command |= BP_PE_COPY_EXECUTE_CLEAR;
    GX_WPAR_BP_LOAD(clear_command);

    // Reload modifyed registers
    if(clear) {
        GX_WPAR_BP_LOAD(GX_BP_REGISTERS_Z_MODE | gx_state.z_mode);
        GX_WPAR_BP_LOAD(GX_BP_REGISTERS_CMODE_0 | gx_state.c_mode_0);
    }
    if(pe_control_dirty) {
        GX_WPAR_BP_LOAD(GX_BP_REGISTERS_PE_CONTROL | gx_state.pe_control);
    }
}

/* -------------------XF Matrix Control--------------------- */

void gx_flash_viewport(float x, float y, float width, float height, float near, float far, bool jitter) {
    // Offset y origin by a sub pixel if jitter
    if(jitter) {
        y -= 0.5f;
    }
    
    float sx = width / 2.0f; // Scale X to 1/2 the width
    float sy = (-height) / 2.0f; // Scale Y to 1/2 and flip

    // Center 0,0 into center of viewport
    float px = x + (width / 2.0f) + 342.0f;
    float py = y + (height / 2.0f) + 342.0f;

    // Normalize Z values between near and far, then scale into the 24 bit depth buffer
    float zFar = far * (float)((1 << 24) - 1);
    float zRange = (far - near) * (float)((1 << 24) - 1);

    // Load values onto XF
    GX_WPAR_XF_LOAD(GX_XF_REGISTER_VIEWPORT, 6);
    GX_WPAR_F32 = sx;
    GX_WPAR_F32 = sy;
    GX_WPAR_F32 = zRange;
    GX_WPAR_F32 = px;
    GX_WPAR_F32 = py;
    GX_WPAR_F32 = zFar;
}

void gx_flash_projection(const matrix4 mtx, bool is_perspective) {
    GX_WPAR_XF_LOAD(GX_XF_REGISTER_PROJECTION, 7);
    GX_WPAR_F32 = mtx[0][0];
    GX_WPAR_F32 = is_perspective ? mtx[0][2] : mtx[0][3];
    GX_WPAR_F32 = mtx[1][1];
    GX_WPAR_F32 = is_perspective ? mtx[1][2] : mtx[1][3];
    GX_WPAR_F32 = mtx[2][2];
    GX_WPAR_F32 = mtx[2][3];
    GX_WPAR_U32 = is_perspective ? 0 : 1;
}

void gx_flash_pos_matrix(const matrix34 mtx, gx_psnmtx_idx index) {
    GX_WPAR_XF_LOAD((uint32_t)index * 4, 12);
    GX_WPAR_F32 = mtx[0][0];
    GX_WPAR_F32 = mtx[0][1];
    GX_WPAR_F32 = mtx[0][2];
    GX_WPAR_F32 = mtx[0][3];
    GX_WPAR_F32 = mtx[1][0];
    GX_WPAR_F32 = mtx[1][1];
    GX_WPAR_F32 = mtx[1][2];
    GX_WPAR_F32 = mtx[1][3];
    GX_WPAR_F32 = mtx[2][0];
    GX_WPAR_F32 = mtx[2][1];
    GX_WPAR_F32 = mtx[2][2];
    GX_WPAR_F32 = mtx[2][3];
}

void gx_flash_nrm_matrix(const matrix3 mtx, gx_psnmtx_idx index) {
    GX_WPAR_XF_LOAD(0x0400 + (uint32_t)index * 3, 9);
    GX_WPAR_F32 = mtx[0][0];
    GX_WPAR_F32 = mtx[0][1];
    GX_WPAR_F32 = mtx[0][2];
    GX_WPAR_F32 = mtx[1][0];
    GX_WPAR_F32 = mtx[1][1];
    GX_WPAR_F32 = mtx[1][2];
    GX_WPAR_F32 = mtx[2][0];
    GX_WPAR_F32 = mtx[2][1];
    GX_WPAR_F32 = mtx[2][2];
}

void gx_set_current_psn_matrix(gx_psnmtx_idx index) {
    gx_state.matrix_index_a &= ~MTXINDX_A_PSN(0b111111);
    gx_state.matrix_index_a |= MTXINDX_A_PSN((uint32_t)index); // Addressed by row.
    gx_state.dirty |= GX_DIRTY_MATRIX_INDEX_NEEDS_UPDATE;
}


/* -------------------XF Color Control--------------------- */

void gx_set_color_channels(uint32_t count) {
    gx_state.genmode = (gx_state.genmode & ~BP_GENMODE_NCOL(0x1F)) | BP_GENMODE_NCOL(count);
    gx_state.dirty |= GX_DIRTY_BP_GENMODE_NEEDS_UPDATE | GX_DIRTY_XF_COLORS_NEEDS_UPDATE;
}

void gx_configure_color_channel(gx_color_channel_t channel, gx_light_bit_t lights,
                                bool lighting, bool ambient_source, bool material_source,
                                gx_diffuse_mode_t diffuse, gx_attenuation_mode_t attenuation) {
    
    uint32_t channel_config = lights;

    if(lighting) // Enable lighting if you please.
        channel_config |= (1<<1);
    if(ambient_source) // Enable ambiant from vertex color
        channel_config |= (1<<6);
    if(material_source) // Enable material from vertex color
        channel_config |= (1<<0);
    
    channel_config |= diffuse << 7;
    channel_config |= attenuation << 9;

    gx_state.xf_color_settings[channel] = channel_config;
    gx_state.dirty |= GX_DIRTY_XF_COLORS_NEEDS_UPDATE;
}

void gx_flash_xf_color(gx_xf_color_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    GX_WPAR_XF_LOAD(id, 1);
    GX_WPAR_U32 = (r << 24) | (g << 16) | (b << 8) | a;
}

void gx_flash_light(gx_light_id_t id, const gx_light_t* light) {
    GX_WPAR_XF_LOAD(id, 16);
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = 0;
    GX_WPAR_U32 = light->color;
    GX_WPAR_F32 = light->cos_attenuation.x;
    GX_WPAR_F32 = light->cos_attenuation.y;
    GX_WPAR_F32 = light->cos_attenuation.z;
    GX_WPAR_F32 = light->distance_attenuation.x;
    GX_WPAR_F32 = light->distance_attenuation.y;
    GX_WPAR_F32 = light->distance_attenuation.z;
    GX_WPAR_F32 = light->position.x;
    GX_WPAR_F32 = light->position.y;
    GX_WPAR_F32 = light->position.z;
    GX_WPAR_F32 = light->direction.x;
    GX_WPAR_F32 = light->direction.y;
    GX_WPAR_F32 = light->direction.z;
}