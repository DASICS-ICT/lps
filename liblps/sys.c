#include "sys.h"
#include "lps_arch.h"
#include "sys/sys.h"

#include <assert.h>

uintptr_t
syshandle(struct LPSThread *t, uintptr_t sysno, uintptr_t a0, uintptr_t a1,
    uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5) 
{
    uintptr_t r = -LINUX_ENOSYS;
    bool handled = false;

    switch (sysno) {
    SYS(set_tid_address, sys_set_tid_address(t, a0))
    SYS(ioctl, sys_ioctl(t, a0, a1, a2, a3, a4, a5))
    SYS(writev, sys_writev(t, a0, a1, a2))
    SYS(exit_group, sys_exit_group(t, a0))
    default:
        LOG(t->proc->engine, "unknown syscall: %ld", sysno);
        ERROR("terminating due to unknown syscall: %ld", sysno);
        exit(1);
    }

    return r;
}

void arch_syshandle(struct LPSContext *ctx)
{
    struct LPSThread *t = lps_ctx_data(ctx);
    assert(t);
    struct LPSRegs *regs = lps_ctx_regs(ctx);

    assert(regs->ucasue == CAUSE_DASICS_U_CHECK_FAULT);
    
    if (regs->dfreason != DFR_EF) {
        switch (regs->dfreason) {
        case DFR_LF:
            LOG(t->proc->engine, "load fault: uepc:%lx, addr:%lx", regs->uepc, regs->utval);
            break;
        case DFR_SF:
            LOG(t->proc->engine, "store fault: uepc:%lx, addr:%lx", regs->uepc, regs->utval);
            break;
        default:
            LOG(t->proc->engine, "DASICS fault(%lx): uepc:%lx, addr:%lx", regs->dfreason, regs->uepc, regs->utval);
        }
        unsigned long base = t->proc->boxinfo.base;
        assert(0);
    }

    // uepc += 4 TODO: 2字节指令对齐支持
    regs->uepc += 4;

    regs->a0 = syshandle(t, regs->a7, regs->a0, regs->a1, regs->a2, regs->a3,
        regs->a4, regs->a5);
}
