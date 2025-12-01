/**
 * @file debugger.c
 * @brief PowerBlocks Debugger
 *
 * Adds extended debugging features.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "powerblocks/core/utils/crash_handler.h"

#include "powerblocks/core/utils/math/vec2.h"

#include "powerblocks/core/graphics/framebuffer.h"
#include "powerblocks/core/graphics/video.h"

#include "powerblocks/core/system/system.h"

#include "qrcodegen.h"

static void rasterize_qr_code(framebuffer_t* framebuffer, uint32_t background, uint32_t foreground, const uint8_t qrcode[], vec2i position) {
	int size = qrcodegen_getSize(qrcode);
	int border = 4;

    const int tile_size = 4;
    vec2i pos = position;

	for (int y = -border; y < size + border; y++) {
        pos.x = position.x;
		for (int x = -border; x < size + border; x++) {
            if(qrcodegen_getModule(qrcode, x, y)) {
                framebuffer_fill_rgba(framebuffer, foreground, pos, vec2i_add(pos, vec2i_new(tile_size, tile_size)));
            } else {
                framebuffer_fill_rgba(framebuffer, background, pos, vec2i_add(pos, vec2i_new(tile_size, tile_size)));
            }

            pos.x += tile_size;
		}
        pos.y += tile_size;
	}
}

static void debugger_crash_handler(const char* cause, exception_context_t *ctx) {
    // Make sure interrupts are disabled
    uint32_t irq_enabled;
    SYSTEM_DISABLE_ISR(irq_enabled);

    // Get currently displayed framebuffer
    framebuffer_t* fb = video_get_framebuffer();
    
    
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    const char *text = "https://bit.ly/AbCd123?d=TestCrashReportSoftware12345QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=";
    enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;

    bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl,
		qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	if (ok)
		rasterize_qr_code(fb, 0x000000FF, 0xFFFFFFFF, qrcode, vec2i_new(VIDEO_WIDTH/2, VIDEO_HEIGHT/2));
}

void debugger_install_crash_handler() {
    crash_handler_set(debugger_crash_handler);
}