/**
 * @file sd.c
 * @brief Implementation of FatFF's disk io.
 *
 * Implementation of FatFF's disk io.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "ff.h"
#include "diskio.h"

#include "powerblocks/core/system/system.h"

#include "powerblocks/filesystem/sd.h"

#include <stdbool.h>

#define DISK_DEV_SD 0

PARTITION VolToPart[] = {
    {0, 1},   // SD card maps physical drive to 1st partition
};

DSTATUS disk_status(BYTE pdrv) {
    switch(pdrv) {
        case DISK_DEV_SD:
            return sd_disk_status();
        default:
            return STA_NOINIT;
    }
}

DSTATUS disk_initialize(BYTE pdrv) {
    switch(pdrv) {
        case DISK_DEV_SD:
            return sd_disk_initialize();
        default:
            return STA_NOINIT;
    }
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    switch(pdrv) {
        case DISK_DEV_SD:
            return sd_disk_read(buff, sector, count);
        default:
            return RES_PARERR;
    }
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    switch(pdrv) {
        case DISK_DEV_SD:
            return sd_disk_write(buff, sector, count);
        default:
            return RES_PARERR;
    }
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    switch(pdrv) {
        case DISK_DEV_SD:
            return sd_disk_ioctl(cmd, buff);
        default:
            return RES_PARERR;
    }
}

#define RTC_EPOCH_YEAR 2000

static const uint8_t days_in_month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

static bool is_leap_year(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

DWORD get_fattime(void) {
    uint64_t tb = system_get_time_base_int();
    uint64_t seconds = tb / SYSTEM_TB_CLOCK_HZ;  // convert ticks to seconds

    // Convert to date/time
    int year = RTC_EPOCH_YEAR;
    int month = 1;
    int day = 1;
    int hour, min, sec;

    sec = seconds % 60;
    seconds /= 60;
    min = seconds % 60;
    seconds /= 60;
    hour = seconds % 24;
    int days = seconds / 24;

    // Compute year
    while (true) {
        int days_in_year = is_leap_year(year) ? 366 : 365;
        if (days >= days_in_year) {
            days -= days_in_year;
            year++;
        } else {
            break;
        }
    }

    // Compute month
    for (int i = 0; i < 12; i++) {
        int dim = days_in_month[i];
        if (i == 1 && is_leap_year(year)) dim++; // Feb in leap year
        if (days >= dim) {
            days -= dim;
            month++;
        } else {
            break;
        }
    }

    day += days;

    // Pack into FAT format
    DWORD fattime = 0;
    fattime |= (DWORD)(year - 1980) << 25;
    fattime |= (DWORD)(month) << 21;
    fattime |= (DWORD)(day) << 16;
    fattime |= (DWORD)(hour) << 11;
    fattime |= (DWORD)(min) << 5;
    fattime |= (DWORD)(sec / 2);

    return fattime;
}