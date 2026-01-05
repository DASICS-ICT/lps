#pragma once

#include "linux.h"
#include "fd.h"
#include "list.h"

#include <pthread.h>

// Maximum number of bytes that can be allocated via sys_brk.
#define BRKMAXSIZE (512UL * 1024 * 1024)

#define STACKSIZE (2UL * 1024 * 1024)

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

struct ElfSeg {
    uintptr_t start;
    uintptr_t end;
    int prot;
};

#define ELFSEGMAXN 4

struct ELFSegInfo {
    struct ElfSeg elfsegs[ELFSEGMAXN];
    int len;
};

struct LPSProc {
    struct LPSBox *box;
    struct LPSBoxInfo boxinfo;

    uintptr_t brkbase;
    size_t brksize;

    uintptr_t entry;
    struct ELFLoadInfo elfinfo;
    struct ELFSegInfo seginfo;

    _Atomic(int) total_thread_count;

    struct FDTable fdtable;

    struct Dir cwd;

    struct LPSLinuxEngine *engine;
};

struct LPSThread {
    struct LPSContext *ctx;

    uintptr_t stack;
    size_t stack_size;

    // Child tid pointer location.
    uintptr_t ctidp;

    int tid;

    enum LPSThreadState{
        THREAD_RUNNABLE,
        THREAD_BOLCKED,
        THREAD_EXITED,
    } state;

    struct List elem;

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

int
proc_chdir(struct LPSProc *p, const char *path);
