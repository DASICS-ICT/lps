#include "arch_asm.h"
#include "core.h"
#include "lps_core.h"
#include "lps_arch.h"
#include "log.h"

#include <stdlib.h>

_Thread_local struct LPSContext *lps_myctx;
_Thread_local struct KRegs lps_kctx;

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
    lps_load_dasics(ctx);
    kswitch(ctx, &lps_kctx, &ctx->kregs);
}

EXPORT void
lps_kswitch_from(struct LPSContext *ctx)
{
    kswitch(NULL, &ctx->kregs, &lps_kctx);
}