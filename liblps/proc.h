#pragma once

#include "linux.h"

#include <pthread.h>


// Maximum number of file descriptors the LFI runtime process can have open.
#define LINUX_NOFILE 1024
// Maximum number of bytes that can be allocated via sys_brk.
#define BRKMAXSIZE (512UL * 1024 * 1024)

struct FDTable {
    // File descriptor conversion table.
    int fds[LINUX_NOFILE];
    // Full sandbox path for opened directories. This is necessary for
    // supporting fchdir(fd).
    char *dirs[LINUX_NOFILE];
    pthread_mutex_t lk;

    bool passthrough;
};

// Information from loading the ELF image.
struct ELFLoadInfo {
    uintptr_t lastva;
    uintptr_t elfentry;
    uintptr_t ldentry;
    uintptr_t elfbase;
    uintptr_t ldbase;
    uint64_t elfphoff;
    uint16_t elfphnum;
    uint16_t elfphentsize;
};

// Used for tracking the current working directory.
struct Dir {
    char path[FILENAME_MAX];
    pthread_mutex_t lk;
};

// Used for tracking sections loaded from the ELF image (dynsym/dynstr).
struct ElfSection {
    uint8_t *data;
    size_t size;
};

struct LPSProc {
    struct LPSBox *box;
    struct LPSBoxInfo boxinfo;

    uintptr_t brkbase;
    size_t brksize;

    uintptr_t entry;
    struct ELFLoadInfo elfinfo;

    _Atomic(int) total_thread_count;

    struct FDTable fdtable;

    struct Dir cwd;

    struct LPSLinuxEngine *engine;
};

struct LPSThread {
    struct LPSContext *ctx;

    uintptr_t stack;
    size_t stack_size;

    int tid;

    struct LPSProc *proc;
};


int
proc_mapany(struct LPSProc *p, size_t size, int prot, int flags, int fd,
    off_t offset, uintptr_t *o_mapstart);

int
proc_mapat(struct LPSProc *p, uintptr_t start, size_t size, int prot,
    int flags, int fd, off_t offset);

int
proc_unmap(struct LPSProc *p, uintptr_t start, size_t size);
