#include "lps_arch.h"
#include "lps_core.h"
#include "core.h"

#include <assert.h>

EXPORT void 
lps_membound_set(struct LPSContext *ctx, int idx, int prot, uintptr_t low, uintptr_t high) 
{
    assert(idx >= 0 && idx < 16);
    ctx->regs.membound[idx][0] = low;
    ctx->regs.membound[idx][1] = high;

    int offset = idx * 4;
    uint64_t mask = ~(0xf << offset);
    uint64_t memcfg = ctx->regs.memcfg & mask;

    ctx->regs.memcfg = memcfg | ((prot | LIBCFG_V) << offset);
}

EXPORT void 
lps_membound_clear(struct LPSContext *ctx, int idx) 
{
    assert(idx >= 0 && idx < 16);
    ctx->regs.membound[idx][0] = 0;
    ctx->regs.membound[idx][1] = 0;

    int offset = idx * 4;
    uint64_t mask = ~(0xf << offset);
    uint64_t memcfg = ctx->regs.memcfg & mask;

    ctx->regs.memcfg = memcfg;
}

EXPORT void 
lps_jmpbound_set(struct LPSContext *ctx, int idx, uintptr_t low, uintptr_t high) 
{
    assert(idx >= 0 && idx < 4);
    ctx->regs.jmpbound[idx][0] = low;
    ctx->regs.jmpbound[idx][1] = high;

    int offset = idx * 4;
    uint64_t mask = ~(0xf << offset);
    uint64_t jmpcfg = ctx->regs.jmpcfg & mask;

    ctx->regs.jmpcfg = jmpcfg | (JMPCFG_V << offset);
}

EXPORT void 
lps_jmpbound_clear(struct LPSContext *ctx, int idx) 
{
    assert(idx >= 0 && idx < 4);
    ctx->regs.jmpbound[idx][0] = 0;
    ctx->regs.jmpbound[idx][1] = 0;

    int offset = idx * 4;
    uint64_t mask = ~(0xf << offset);
    uint64_t jmpcfg = ctx->regs.jmpcfg & mask;

    ctx->regs.jmpcfg = jmpcfg;
}