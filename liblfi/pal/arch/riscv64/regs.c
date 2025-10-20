#include "lfi_arch.h"
#include "pal/platform.h"
#include "pal/arch/arm64/regs.h"

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
