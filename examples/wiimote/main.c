#include "powerblocks/core/system/system.h"
#include "powerblocks/core/ios/ios.h"
#include "powerblocks/core/ios/ios_settings.h"

#include "powerblocks/core/graphics/video.h"

#include "powerblocks/core/utils/fonts.h"
#include "powerblocks/core/utils/console.h"

#include "powerblocks/core/bluetooth/bltootls.h"

#include "powerblocks/input/wiimote/wiimote.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <math.h>
#include <string.h>

framebuffer_t frame_buffer ALIGN(512);

void retrace_callback() {
    // Make it so we can see the framebuffer changes
    system_flush_dcache(&frame_buffer, sizeof(frame_buffer));
}

typedef struct {
    bool view_accelerometer;
    bool view_ir;
    wiimote_extension_t last_extension_type;
    bool view_nunchuck_analog;
    bool view_classic_analog;
} example_state_t;

static example_state_t example_state[4];

static void extension_connect_message(wiimote_extension_t type) {
    const char* name = wiimote_extension_get_name(type);

    switch(type) {
        case WIIMOTE_EXTENSION_NUNCHUK:
            printf("Press C on the nunchuck to view accelerometer and analog stick.\n");
            break;
        case WIIMOTE_EXTENSION_CLASSIC_CONTROLLER:
            printf("Press A on the classic controller to view analog data.\n");
            break;
        default:
            break;
    }
}

static void do_nunchuck(int number) {
    // Alert of button changes
    if(WIIMOTES[number].extensions.nunchuck.buttons.down & WIIMOTE_EXTENSION_NUNCHUCK_BUTTONS_Z)
        printf("Nunchuck %d, Z down\n", number);
    if(WIIMOTES[number].extensions.nunchuck.buttons.down & WIIMOTE_EXTENSION_NUNCHUCK_BUTTONS_C)
        printf("Nunchuck %d, C down\n", number);
    
    if(WIIMOTES[number].extensions.nunchuck.buttons.up & WIIMOTE_EXTENSION_NUNCHUCK_BUTTONS_Z)
        printf("Nunchuck %d, Z up\n", number);
    if(WIIMOTES[number].extensions.nunchuck.buttons.up & WIIMOTE_EXTENSION_NUNCHUCK_BUTTONS_C)
        printf("Nunchuck %d, C up\n", number);
    
    // Toggles
    if(WIIMOTES[number].extensions.nunchuck.buttons.down & WIIMOTE_EXTENSION_NUNCHUCK_BUTTONS_C) {
        example_state[number].view_nunchuck_analog ^= true;
    }

    if(example_state[number].view_nunchuck_analog) {
        printf("Nunchuck: Stick: (%f, %f), Accelerometer: (%f, %f %f)\n",
            WIIMOTES[number].extensions.nunchuck.stick.x,
            WIIMOTES[number].extensions.nunchuck.stick.y,
            WIIMOTES[number].extensions.nunchuck.accelerometer.x,
            WIIMOTES[number].extensions.nunchuck.accelerometer.y,
            WIIMOTES[number].extensions.nunchuck.accelerometer.z);
    }
}

static void do_classic_controller(int number) {
    const wiimote_extension_data_t* ext = &WIIMOTES[number].extensions;
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_UP)
        printf("Classic %d, dpad up down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_LEFT)
        printf("Classic %d, dpad left down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Z_RIGHT)
        printf("Classic %d, Z right down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_X)
        printf("Classic %d, X down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_A)
        printf("Classic %d, A down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Y)
        printf("Classic %d, Y down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_B)
        printf("Classic %d, B down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Z_LEFT)
        printf("Classic %d, Z left down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_RIGHT_TRIGGER)
        printf("Classic %d, R Trigger down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_PLUS)
        printf("Classic %d, + down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_HOME)
        printf("Classic %d, Home down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_MINUS)
        printf("Classic %d, Minus down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_LEFT_TRIGGER)
        printf("Classic %d, Left Trigger down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_DOWN)
        printf("Classic %d, dpad down down\n", number);
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_RIGHT)
        printf("Classic %d, dpad right down\n", number);
    
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_UP)
        printf("Classic %d, dpad up up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_LEFT)
        printf("Classic %d, dpad left up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Z_RIGHT)
        printf("Classic %d, Z right up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_X)
        printf("Classic %d, X up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_A)
        printf("Classic %d, A up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Y)
        printf("Classic %d, Y up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_B)
        printf("Classic %d, B up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Z_LEFT)
        printf("Classic %d, Z left up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_RIGHT_TRIGGER)
        printf("Classic %d, R Trigger up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_PLUS)
        printf("Classic %d, + up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_HOME)
        printf("Classic %d, Home up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_MINUS)
        printf("Classic %d, Minus up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_LEFT_TRIGGER)
        printf("Classic %d, Left Trigger up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_DOWN)
        printf("Classic %d, dpad down up\n", number);
    if(ext->classic_controller.buttons.up & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_RIGHT)
        printf("Classic %d, dpad right up\n", number);

    // Toggles
    if(ext->classic_controller.buttons.down & WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_A) {
        example_state[number].view_classic_analog ^= true;
    }

    if(example_state[number].view_classic_analog) {
        printf("LStick:  (%f, %f)\n", ext->classic_controller.left_stick.x, ext->classic_controller.left_stick.y);
        printf("RStick:  (%f, %f)\n", ext->classic_controller.right_stick.x, ext->classic_controller.right_stick.y);
        printf("Trigger: (%f, %f)\n", ext->classic_controller.triggers.x, ext->classic_controller.triggers.y);
    }
}

static void do_wiimote(int number) {
    // If this wiimote's driver is not loaded, its not connected
    if(WIIMOTES[number].driver == NULL)
        return;

    // Draw the wiimote cursor
    vec2i size = vec2i_new(5, 5);
    vec2i cursor = WIIMOTES[number].cursor.pos;
    framebuffer_fill_rgba(&frame_buffer, 0xFF0000FF, vec2i_sub(cursor, size), vec2i_add(cursor, size));

    // Alert us of any button changes.
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_DPAD_LEFT) printf("Remote %d, dpad left down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_DPAD_RIGHT) printf("Remote %d, dpad right down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_DPAD_DOWN) printf("Remote %d, dpad down down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_DPAD_UP) printf("Remote %d, dpad up down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_PLUS) printf("Remote %d, plus down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_TWO) printf("Remote %d, two down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_ONE) printf("Remote %d, one down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_B) printf("Remote %d, B down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_A) printf("Remote %d, A down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_MINUS) printf("Remote %d, minus down\n", number);
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_HOME) printf("Remote %d, home down\n", number);
    
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_DPAD_LEFT) printf("Remote %d, dpad left up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_DPAD_RIGHT) printf("Remote %d, dpad right up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_DPAD_DOWN) printf("Remote %d, dpad down up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_DPAD_UP) printf("Remote %d, dpad up up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_PLUS) printf("Remote %d, plus up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_TWO) printf("Remote %d, two up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_ONE) printf("Remote %d, one up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_B) printf("Remote %d, B up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_A) printf("Remote %d, A up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_MINUS) printf("Remote %d, minus up\n", number);
    if(WIIMOTES[number].buttons.up & WIIMOTE_BUTTONS_HOME) printf("Remote %d, home up\n", number);

    // Toggles
    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_A) {
        example_state[number].view_accelerometer ^= true;
    }

    if(WIIMOTES[number].buttons.down & WIIMOTE_BUTTONS_B) {
        example_state[number].view_ir ^= true;
    }

    // View accelerometer
    if(example_state[number].view_accelerometer) {
        printf("Accelerometer:\n");
        printf("  Rectangular: %f, %f, %f\n", WIIMOTES[number].accelerometer.rectangular.x,
                                              WIIMOTES[number].accelerometer.rectangular.y,
                                              WIIMOTES[number].accelerometer.rectangular.z);
        printf("  Spherical:   %f, %f, %f\n", WIIMOTES[number].accelerometer.spherical.x,
                                              WIIMOTES[number].accelerometer.spherical.y,
                                              WIIMOTES[number].accelerometer.spherical.z);
        printf("  Orientation: %f, %f, %f\n", WIIMOTES[number].accelerometer.orientation.x,
                                              WIIMOTES[number].accelerometer.orientation.y,
                                              WIIMOTES[number].accelerometer.orientation.z);
    }

    // View IR
    if(example_state[number].view_ir) {
        printf("Cursor: Pos: (%d, %d), Distance: %f, Z: %f, Yaw: %f\n",
            WIIMOTES[number].cursor.pos.x, WIIMOTES[number].cursor.pos.y,
            WIIMOTES[number].cursor.distance,
            WIIMOTES[number].cursor.z,
            WIIMOTES[number].cursor.yaw);
        for(int i = 0; i < 4; i++) {
            if(!WIIMOTES[number].ir_tracking[i].visible)
                return;
            
            printf("IR Point %d, Side: %s, Pos: (%d, %d)\n",
                i, WIIMOTES[number].ir_tracking[i].side ? "right" : "left",
                WIIMOTES[number].ir_tracking[i].position.x, WIIMOTES[number].ir_tracking[i].position.y);
        }
    }

    // Detect new extension
    if(example_state[number].last_extension_type != WIIMOTES[number].extensions.type) {
        if(WIIMOTES[number].extensions.type == WIIMOTE_EXTENSION_NONE) {
            printf("Extension unplugged.\n");
        } else {
            extension_connect_message(WIIMOTES[number].extensions.type);
        }
    }
    example_state[number].last_extension_type = WIIMOTES[number].extensions.type;

    // Handle extensions
    switch(WIIMOTES[number].extensions.type) {
        case WIIMOTE_EXTENSION_NUNCHUK:
            do_nunchuck(number);
            break;
        case WIIMOTE_EXTENSION_CLASSIC_CONTROLLER:
            do_classic_controller(number);
            break;
        default:
            break;
    }
}

int main() {
    // Initialize IOS. Must be done first as many thing use it
    ios_initialize();

    // Get default video mode from IOS and use it to initialize the video interface.
    video_mode_t tv_mode = video_system_default_video_mode();
    video_initialize(tv_mode);

    // Fill background with black
    console_initialize(&frame_buffer, &fonts_ibm_iso_8x16);
    video_set_framebuffer(&frame_buffer);
    video_set_retrace_callback(retrace_callback);

    // Create Blue Background
    framebuffer_fill_rgba(&frame_buffer, 0x000000FF, vec2i_new(0,0), vec2i_new(VIDEO_WIDTH, VIDEO_HEIGHT));

    // Back Text To Black, Blue Background
    console_set_text_color(0xFFFFFFFF, 0x000000FF);

    // print super cool hello message
    printf("\n\n\n");
    printf("  PowerBlocks SDK Wiimote Example\n");
    printf("  Tap (A) on wiimote to toggle accelerometer data.\n");
    printf("  Tap (B) on wiimote to toggle IR data.\n");
    memset(example_state, 0, sizeof(example_state));

    int ret = bltools_initialize();
    if(ret < 0) {
        printf("Failed To Init Bluetooth.\n");
    }

    wiimotes_initialize();

    printf("Begining Discovery!\n");
    ret = bltools_begin_automatic_discovery(portMAX_DELAY);
    if(ret < 0) {
        printf("Auto discovery failed.\n");
    }


    while(true) {
        wiimote_poll();

        vec2i cursor = WIIMOTES[0].cursor.pos;

        // Do all 4 wiimotes
        for(int i = 0; i < 4; i++) {
            do_wiimote(i);
        }

        // Wait for vsync
        video_wait_vsync();
    }

    return 0;
}