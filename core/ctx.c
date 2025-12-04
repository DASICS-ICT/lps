#include "arch_asm.h"
#include "core.h"
#include "lps_core.h"
#include "lps_arch.h"
#include "log.h"

#include <stdlib.h>

_Thread_local struct LPSContext *lps_myctx;

extern void lps_ctx_entry(struct LPSContext* ctx)
    asm ("lps_ctx_entry");

static void kregs_init(struct KRegs *kregs, uintptr_t entry, uintptr_t sp, uintptr_t sp_base) 
{
    kregs->sp = sp;
    kregs->sp_base = sp_base;
    kregs->ra = entry;
    LOG_("kregs_init: entry:%lx, sp:%lx, sp_base:%lx", entry, sp, sp_base);
}

EXPORT struct LPSContext *
lps_ctx_new(struct LPSBox *box, void *userdata)
{
    struct LPSContext *ctx = malloc(sizeof(struct LPSContext));
    if (!ctx) {
        return NULL;
    }

    ctx->userdata = userdata;
    ctx->box = box;

    void *kstack = malloc(2 * 1024 * 1024);
    if (!kstack) {

    }
    void *kstackp = kstack + 2 * 1024 * 1024;

    kregs_init(&ctx->kregs, (uintptr_t)lps_ctx_entry, (uintptr_t)kstackp, (uintptr_t)kstack);

    return ctx;
}

EXPORT struct LPSRegs *
lps_ctx_regs(struct LPSContext *ctx)
{
    return &ctx->regs;
}

EXPORT struct LPSContext *
lps_cur_ctx(void) {
    return lps_myctx;
}

EXPORT void
lps_set_ctx(struct LPSContext *ctx) {
    lps_myctx = ctx;
}
