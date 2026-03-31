#include "arch_asm.h"
#include "core.h"
#include "lps_core.h"
#include "lps_arch.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>

_Thread_local struct LPSContext *lps_myctx;
_Thread_local struct KRegs lps_kctx;

/* ---- DASICS CSR breakdown profiling ---- */
static inline uint64_t rdcycle_ctx(void) {
    uint64_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
}

_Thread_local uint64_t lps_dasics_csr_cycles = 0;
_Thread_local uint64_t lps_switch_count = 0;

void lps_switch_stats_reset(void) {
    lps_dasics_csr_cycles = 0;
    lps_switch_count = 0;
}

void lps_switch_stats_print(void) {
    if (lps_switch_count == 0) {
        printf("[LPS-PROF] no context switches recorded\n");
        return;
    }
    printf("[LPS-PROF] context switches: %lu\n", lps_switch_count);
    printf("[LPS-PROF] DASICS CSR total cycles: %lu\n", lps_dasics_csr_cycles);
    printf("[LPS-PROF] DASICS CSR avg cycles/switch: %lu\n",
           lps_dasics_csr_cycles / lps_switch_count);
}
/* ---- end profiling ---- */

extern void lps_ctx_entry(struct LPSContext *ctx)
    __asm__ ("lps_ctx_entry");

extern void kswitch(struct LPSContext *ctx, struct KRegs *old, struct KRegs *new)
    __asm__ ("kswitch");

extern void lps_load_dasics(struct LPSContext* ctx)
    __asm__ ("lps_load_dasics");

static void kregs_init(struct KRegs *kregs, uintptr_t entry, uintptr_t sp, uintptr_t sp_base) 
{
    kregs->sp = sp;
    kregs->sp_base = sp_base;
    kregs->ra = entry;
    // LOG_("kregs_init: entry:%lx, sp:%lx, sp_base:%lx", entry, sp, sp_base);
}

EXPORT struct LPSContext *
lps_ctx_new(struct LPSBox *box, void *userdata)
{
    struct LPSContext *ctx = malloc(sizeof(struct LPSContext));
    if (!ctx) {
        return NULL;
    }

    *ctx = (struct LPSContext) {
        .userdata = userdata,
        .box = box,
    };

    void *kstack = malloc(2 * 1024 * 1024);
    if (!kstack) {
        return NULL;
    }
    void *kstackp = kstack + 2 * 1024 * 1024;

    kregs_init(&ctx->kregs, (uintptr_t)lps_ctx_entry, (uintptr_t)kstackp, (uintptr_t)kstack);

    // TODO: tmp dasics whole range
    lps_membound_set(ctx, 0, LIBCFG_R | LIBCFG_W, box->base, box->base + box->size);
    lps_jmpbound_set(ctx, 0, box->base, box->base + box->size);

    return ctx;
}

EXPORT struct LPSRegs *
lps_ctx_regs(struct LPSContext *ctx)
{
    return &ctx->regs;
}

EXPORT struct LPSContext *
lps_cur_ctx(void) 
{
    return lps_myctx;
}

EXPORT void
lps_set_ctx(struct LPSContext *ctx) 
{
    lps_myctx = ctx;
}

EXPORT void
lps_ctx_free(struct LPSContext *ctx) 
{
    free(ctx);
}

EXPORT void
lps_kswitch_to(struct LPSContext *ctx)
{
    lps_myctx = ctx;
    uint64_t t0 = rdcycle_ctx();
    lps_load_dasics(ctx);
    uint64_t t1 = rdcycle_ctx();
    lps_dasics_csr_cycles += (t1 - t0);
    lps_switch_count++;
    kswitch(ctx, &lps_kctx, &ctx->kregs);
}

EXPORT void
lps_kswitch_from(struct LPSContext *ctx)
{
    kswitch(NULL, &ctx->kregs, &lps_kctx);
}