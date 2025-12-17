#pragma once

#include "linux.h"

#include <pthread.h>

struct FDFile {
    void *dev;
    size_t refs;
    pthread_mutex_t lk_refs;

    ssize_t (*read)(void*, uint8_t*, size_t);
    ssize_t (*write)(void*, uint8_t*, size_t);
    ssize_t (*lseek)(void*, linux_off_t, int);
    int     (*close)(void*);
    int     (*stat_)(void*, struct Stat*);
    ssize_t (*getdents)(void*, void*, size_t);
    int     (*chown)(void*, linux_uid_t, linux_gid_t);
    int     (*chmod)(void*, linux_mode_t);
    int     (*truncate)(void*, linux_off_t);
    int     (*sync)(void*);
    int     (*ioctl)(void*, unsigned long, void *);

    int kfd;
    char *path;
};

// Generate a FDFile datat structure from kfd
struct FDFile *
filenew(int kfd, char *path);

// Free FDFile structure, free path and close kfd
void
filefree(struct FDFile *f);