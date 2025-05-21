#include "crash_handler.h"

#include "graphics/video.h"

#include "system/system.h"

#include "fonts.h"

#include <stdio.h>

#define BORDERS 64
#define font fonts_ibm_iso_8x16

#define TEXT_BORDER 8

#define HORIZONTAL_SPACING ((VIDEO_WIDTH - (BORDERS+TEXT_BORDER) * 2) / 4)

static crash_handler_t system_crash_handler = NULL;

void crash_handler_bug_check(const char* cause, exception_context_t* ctx) {
    if(system_crash_handler) {
        system_crash_handler(cause, ctx);
        return;
    }
    
    // Make sure interrupts are disabled
    uint32_t irq_enabled;
    SYSTEM_DISABLE_ISR(irq_enabled);

    // Get currently displayed framebuffer
    framebuffer_t* fb = video_get_framebuffer();

    // Darken background.
    // Such that the user can still see the game when it crashed to aid
    // knowing where the game crashed.
    framebuffer_fill_rgba(fb, 0x000000C0, vec2i_new(BORDERS,BORDERS), vec2i_new(VIDEO_WIDTH-BORDERS, VIDEO_HEIGHT-BORDERS));

    vec2i text_pos = vec2i_new(BORDERS + TEXT_BORDER, BORDERS + TEXT_BORDER);
    
    framebuffer_put_text(fb, 0xFFFFFFFF, 0x00000000, text_pos, &font, cause);
    text_pos.y += font.character_size.y;

    if(ctx) {
        char buffer[32];

        sprintf(buffer, "PC: %08XH", ctx->srr0);
        framebuffer_put_text(fb, 0xFFFFFFFF, 0x00000000, text_pos, &font, buffer);

        text_pos.y += font.character_size.y * 2;

        uint32_t registers[32] = {
            ctx->r0, ctx->stack_frame, ctx->r2, ctx->r3,
            ctx->r4, ctx->r5, ctx->r6, ctx->r7,
            ctx->r8, ctx->r9, ctx->r10, ctx->r11,
            ctx->r12, ctx->r13, ctx->r14, ctx->r15,
            ctx->r16, ctx->r17, ctx->r18, ctx->r19,
            ctx->r20, ctx->r21, ctx->r22, ctx->r23,
            ctx->r24, ctx->r25, ctx->r26, ctx->r27,
            ctx->r28, ctx->r29, ctx->r30, ctx->r31,
        };

        for(int y = 0; y < 8; y++) {
            for(int x = 0; x < 4; x++) {
                int index = y + x * 8;
                sprintf(buffer, "R%02d:%08XH", index, registers[index]);
                framebuffer_put_text(fb, 0xFFFFFFFF, 0x00000000, vec2i_new(text_pos.x + HORIZONTAL_SPACING * x, text_pos.y), &font, buffer);
            }

            text_pos.y += font.character_size.y;
        }

        text_pos.y += font.character_size.y;

        for(int y = 0; y < 8; y++) {
            for(int x = 0; x < 4; x++) {
                int index = y + x * 8;
                sprintf(buffer, "F%02d:%+1.4E", index, ctx->f[index]);
                //framebuffer_put_text(fb, 0xFFFFFFFF, 0x00000000, vec2i_new(text_pos.x + HORIZONTAL_SPACING * x, text_pos.y), &font, buffer);
            }

            text_pos.y += font.character_size.y;
        }
    }

    // Flush framebuffer
    system_flush_dcache(fb, sizeof(*fb));

    // Infinite Loop
    while(1);
}

void crash_handler_set(crash_handler_t handler) {
    system_crash_handler = handler;
}
