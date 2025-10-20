#include <assert.h>
#include <stdint.h>

#include "lfi.h"
#include "sys.h"
#include "syscalls/strace.h"
#include "syscalls/syscalls.h"
#include "pal/platform.h"

#include "arch_sys.h"
#include "print.h"

void
arch_syshandle(struct LFIContext* ctx)
{
    struct TuxThread* p = (struct TuxThread*) lfi_ctx_data(ctx);
    struct TuxRegs* regs = lfi_ctx_regs(ctx);
    
    if (regs->dfreason != 1) {
        switch (regs->dfreason) {
        case 2:
            DBG("load fault: uepc:%lx, addr:%lx", regs->uepc, regs->utval);
        case 3:
            DBG("store fault: uepc:%lx, addr:%lx", regs->uepc, regs->utval);
        }
        unsigned long base = p->proc->p_info.base;
        DBG("elf load base:%lx", base);
        assert(0);
    }

    // uepc += 4 TODO: 2字节指令对齐支持
    regs->uepc += 4;

    switch (regs->a7) {
    default:
        // Generic syscalls.
        regs->a0 = syshandle(p, regs->a7, regs->a0, regs->a1, regs->a2,
                regs->a3, regs->a4, regs->a5);
    }
}
