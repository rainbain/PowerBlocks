#include "powerblocks/core/system/system.h"
#include "powerblocks/core/ios/ios.h"
#include "powerblocks/core/ios/ios_settings.h"

#include "powerblocks/core/graphics/video.h"

#include "powerblocks/core/utils/fonts.h"
#include "powerblocks/core/utils/console.h"

#include "powerblocks/filesystem/sd.h"

#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include "ff.h"

framebuffer_t frame_buffer ALIGN(512);

void retrace_callback() {
    // Make it so we can see the framebuffer changes
    system_flush_dcache(&frame_buffer, sizeof(frame_buffer));
}

FATFS fs;

static void write_time_announcement(FILE* fp) {
    fprintf(fp, "Hello World From PowerBlocks!\n");

    uint64_t total_ms = system_get_time_base_int() / (SYSTEM_TB_CLOCK_HZ / 1000);

    int ms = total_ms % 1000;
    int s = (total_ms / 1000) % 60;
    int m = (total_ms / 60000) % 60;
    int h_24 = (total_ms / 3600000) % 24;

    fprintf(fp, "BOINNG! BOINNG!\nThe current time is %02d:%02d!\n", h_24, m);
}

static void write_gossip(FILE* fp) {
    fprintf(fp, "They say that Gerudos sometimes\n");
    fprintf(fp, "come to Hyrule Castle Town to\n");
    fprintf(fp, "look for boyfriends\n");
}

static void file_system_example() {
    FRESULT fr;
    // Lets list the directories
    DIR dir;
    fr = f_opendir(&dir, ".");
    if(fr != FR_OK) {
        printf("Failed to open directory. Error: %d!\n", fr);
        return;
    }

    FILINFO fno;
    while(true) {
        fr = f_readdir(&dir, &fno);

        // Break on error or last file
        if(fr != FR_OK || fno.fname[0] == 0)
            break;
        
        if(fno.fattrib & AM_DIR) {
            printf("<DIR>  %s\n", fno.fname);
        } else {
            printf("<FILE> %s (%d bytes)\n", fno.fname, fno.fsize);
        }
    }

    // Check for mask of truth
    bool truth_exist = f_stat("truth.txt", &fno) == FR_OK;


    // Lets create a new file using libc
    printf("Creating Hello World.txt\n");
    FILE *fp = fopen("Hello World.txt", "w");
    if(!fp) {
        printf("Failed to open for writing.\n");
        return;
    }

    // Write text
    if(!truth_exist) {
        write_time_announcement(fp);
    } else {
        write_gossip(fp);
    }

    // Were done here
    fclose(fp);

    // Open it for reading
    printf("Reading Hello World.txt\n");
    fp = fopen("Hello World.txt", "r");
    if (!fp) {
        printf("Failed to open for reading.\n");
        return;
    }

    char buffer[256];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytesRead] = '\0';  // null-terminate

    fclose(fp);

    printf("Read Back:\n%s", buffer);
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

    // Set the retrace callback to flush the framebuffer before drawing.
    video_set_retrace_callback(retrace_callback);

    // Create Blue Background
    framebuffer_fill_rgba(&frame_buffer, 0x000000FF, vec2i_new(0,0), vec2i_new(VIDEO_WIDTH, VIDEO_HEIGHT));

    // Back Text To Black, Blue Background
    console_set_text_color(0xFFFFFFFF, 0x000000FF);

    // print super cool hello message
    printf("\n\n\n");
    printf("  PowerBlocks SDK\n");
    printf("  Hello World this is a Wii.\n");


    // Initialize the SD card interface
    sd_initialize();

    // Mount the file system, in this case "" defaults to SD.
    FRESULT fr = f_mount(&fs, "0:", 1);
    if(fr != FR_OK) {
        printf("Failed to mount. Error: %d!\n", fr);
        goto ERROR;
    }

    // Get the folder the game was launched from, on the SD card
    char game_dir[30];
    system_get_boot_path("sd", game_dir, sizeof(game_dir));

    // Change dir to it, and run the example.
    printf("Game launched from: %s\n", game_dir);
    chdir(game_dir);
    file_system_example();
ERROR:

    while(true) {
        // Wait for vsync
        video_wait_vsync();
    }

    return 0;
}