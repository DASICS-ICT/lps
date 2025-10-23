#include "lfi_arch.h"
#include "pal/platform.h"
#include "pal/arch/arm64/regs.h"

#include "print.h"

EXPORT void
lfi_regs_init(struct TuxRegs* regs, struct LFIAddrSpace* as, struct LFIContext* ctx)
{
    // INIT DASICS BOUND
    regs->membound[0][0] = as->minaddr;
    regs->membound[0][1] = as->maxaddr;
    regs->jmpbound[0][0] = as->minaddr;
    regs->jmpbound[0][1] = as->maxaddr;
    regs->memcfg = 0xb;
    regs->jmpcfg = 0x1;
}

EXPORT void
lfi_ctx_init_sys(struct LFIContext* ctx)
{
}

uintptr_t*
lfi_regs_entry(struct TuxRegs* regs)
{
    return NULL;
}

uintptr_t*
lfi_regs_arg0(struct TuxRegs* regs)
{
    return (uintptr_t*) &regs->a0;
}

// entry: the entrance of first running
// sp: kernel stack pointer
// sp_base: kernel stack base
EXPORT void
kcontext_init(struct KContext* k_ctx, uintptr_t entry, uintptr_t sp, uintptr_t sp_base) {
    k_ctx->sp = sp;
    k_ctx->sp_base = sp_base;
    k_ctx->ra = entry;
    DBG("[kcontext_init]: entry:%lx, sp:%lx, sp_base:%lx", entry, sp, sp_base);
}
