#include <assert.h>
#include <stdint.h>

#include "lfi.h"
#include "sys.h"
#include "syscalls/strace.h"
#include "syscalls/syscalls.h"
#include "pal/platform.h"

#include "arch_sys.h"
#include "print.h"

#include "pal/arch/riscv64/regs.h"

inline static uint64_t get_time(void) {
    uint64_t time;
    asm volatile ("rdtime %0" : "=r"(time));
    return time;
}

void
arch_syshandle(struct LFIContext* ctx)
{
    struct TuxThread* p = (struct TuxThread*) lfi_ctx_data(ctx);
    struct TuxRegs* regs = lfi_ctx_regs(ctx);

    if (regs->ucasue == CAUSE_IRQ_U_TIME) {
        // TODO: check overflow
        uint64_t next = get_time() + 10000000;
		DBG("[U_INTR_HANDLER] set utimecmp to %lu", next);
		csr_write(CSR_UTIMECMP, next);
        return;
    }

    assert(regs->ucasue == CAUSE_DASICS_U_CHECK_FAULT);
    
    if (regs->dfreason != DFR_EF) {
        switch (regs->dfreason) {
        case DFR_LF:
            DBG("load fault: uepc:%lx, addr:%lx", regs->uepc, regs->utval);
            break;
        case DFR_SF:
            DBG("store fault: uepc:%lx, addr:%lx", regs->uepc, regs->utval);
            break;
        default:
            DBG("DASICS fault(%lx): uepc:%lx, addr:%lx", regs->dfreason, regs->uepc, regs->utval);
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
