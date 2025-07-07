#include <assert.h>
#include <stdint.h>

#include "lfi.h"
#include "sys.h"
#include "syscalls/strace.h"
#include "syscalls/syscalls.h"
#include "pal/platform.h"

#include "arch_sys.h"

void
arch_syshandle(struct LFIContext* ctx)
{
    struct TuxThread* p = (struct TuxThread*) lfi_ctx_data(ctx);
    struct TuxRegs* regs = lfi_ctx_regs(ctx);

    switch (regs->a7) {
    default:
        // Generic syscalls.
        regs->a0 = syshandle(p, regs->a7, regs->a0, regs->a1, regs->a2,
                regs->a3, regs->a4, regs->a5);
    }
}
