#include <assert.h>

#include "arch_regs.h"

void
regs_init(struct TuxRegs* regs, uintptr_t entry, uintptr_t sp)
{
    regs->sp = sp;
    regs->ra = entry;
}

uintptr_t*
regs_return(struct TuxRegs* regs)
{
    return &regs->a0;
}

uintptr_t*
regs_sp(struct TuxRegs* regs)
{
    return &regs->sp;
}
