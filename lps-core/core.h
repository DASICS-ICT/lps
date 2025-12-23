#pragma once

#include "boxmap.h"
#include "lps_arch.h"
#include "lps_core.h"
#include "mmap.h"

#define EXPORT __attribute__((visibility("default")))

struct LPSEngine {
    struct BoxMap *bm;

    struct LPSOptions opts;
    
    void (*sys_handler)(struct LPSContext *ctx);
};

struct LPSBox {
    struct MMAddrSpace mm;

    uintptr_t base;
    size_t size; 

    void *userdata;

    struct LPSEngine *engine;
};

struct LPSContext {
    struct LPSRegs regs;
    
    struct KRegs kregs;

    void *userdata;

    struct LPSBox *box;
};