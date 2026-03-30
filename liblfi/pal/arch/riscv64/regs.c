#include "lfi_arch.h"
#include "pal/platform.h"
#include "pal/arch/arm64/regs.h"

EXPORT void
lfi_regs_init(struct TuxRegs* regs, struct LFIAddrSpace* as, struct LFIContext* ctx)
{
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
