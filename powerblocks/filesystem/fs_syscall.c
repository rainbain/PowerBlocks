/**
 * @file fs_syscall.c
 * @brief Implements libc's filesystem syscalls
 *
 * Implements libc's filesystem syscalls
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <fcntl.h>

#include "ff.h"

typedef struct {
    uint8_t used;
    FIL fil;
} file_descriptor_t;

static size_t descriptor_table_size = 0;
static file_descriptor_t* file_descriptor_table = NULL;

// Find and mark an entry as used, or allocate a new one if needed
static int allocate_file() {
    for(int i = 0; i < descriptor_table_size; i++) {
        if(file_descriptor_table[i].used == 0) {
            file_descriptor_table[i].used = 1;
            return i;
        }
    }

    // Allocate a new entry
    int entry = descriptor_table_size;

    file_descriptor_t* new_table = (file_descriptor_t*)realloc(file_descriptor_table, (descriptor_table_size + 1) * sizeof(*file_descriptor_table));
    if(new_table == NULL) { // oh deer, out of memory
        return -1;
    }

    descriptor_table_size++;
    file_descriptor_table = new_table;

    file_descriptor_table[entry].used = 1;
    return entry;
}

static void free_file(int i) {
    file_descriptor_table[i].used = 0;
}

int open(const char *path, int flags, ...) {
    BYTE fatfs_mode = 0;

    switch (flags & O_ACCMODE) {
        case O_RDONLY:
            fatfs_mode |= FA_READ;
            break;

        case O_WRONLY:
            fatfs_mode |= FA_WRITE;
            break;

        case O_RDWR:
            fatfs_mode |= FA_READ | FA_WRITE;
            break;
    }

    if (flags & O_CREAT) {
        if (fatfs_mode & FA_WRITE) {
            // Only ok if opened for writing
            fatfs_mode |= FA_OPEN_ALWAYS;
        } else {
            errno = EINVAL;
            return -1;
        }
    }

    if (flags & O_TRUNC) {
        if (fatfs_mode & FA_WRITE) {
            fatfs_mode |= FA_CREATE_ALWAYS;
        } else {
            errno = EINVAL;
            return -1;
        }
    }

    if (flags & O_APPEND)
        fatfs_mode |= FA_OPEN_APPEND;
    
    /// BUG FIX: Avoid passing flags of zero
    // Doing that bufs out fatfs
    if(fatfs_mode == 0) {
        errno = EINVAL;
        return -1;
    }
    

    int fd = allocate_file();
    if(fd < 0) {
        errno = EMFILE; // Too many open files
        return fd;
    }

    FRESULT res = f_open(&file_descriptor_table[fd].fil, path, fatfs_mode);
    if(res != FR_OK) {
        free_file(fd);
        errno = EIO;
        return -1;
    }

    return fd;
}

ssize_t read(int fd, void* buf, size_t count) {
    if(fd < 0 || fd >= descriptor_table_size || file_descriptor_table[fd].used == 0) {
        errno = EBADF;
        return -1;
    }

    UINT br = 0;
    FRESULT res = f_read(&file_descriptor_table[fd].fil, buf, count, &br);
    if(res != FR_OK) {
        errno = EIO;
        return -1;
    }

    return br;
}

ssize_t write(int fd, const void* buf, size_t count) {
    if(fd < 0 || fd >= descriptor_table_size || file_descriptor_table[fd].used == 0) {
        errno = EBADF;
        return -1;
    }

    UINT bw = 0;
    FRESULT res = f_write(&file_descriptor_table[fd].fil, buf, count, &bw);
    if(res != FR_OK) {
        errno = EIO;
        return -1;
    }

    return bw;
}

off_t lseek(int fd, off_t offset, int whence) {
    if(fd < 0 || fd >= descriptor_table_size || file_descriptor_table[fd].used == 0) {
        errno = EBADF;
        return -1;
    }

    FIL *fp = &file_descriptor_table[fd].fil;
    DWORD new_pos;

    switch(whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = f_tell(fp) + offset;
            break;;
        case SEEK_END:
            new_pos = f_size(fp) + offset;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    FRESULT res = f_lseek(fp, new_pos);
    if(res != FR_OK) {
        errno = EIO;
        return -1;
    }

    return new_pos;
}

int close(int fd) {
    if(fd < 0 || fd >= descriptor_table_size || file_descriptor_table[fd].used == 0) {
        errno = EBADF;
        return -1;
    }

    f_close(&file_descriptor_table[fd].fil);
    free_file(fd);
    return 0;
}

int fstat(int fd, struct stat *st) {
    st->st_mode = S_IFREG;
    return 0;
}

int isatty(int fd) {
    return 0;
}

