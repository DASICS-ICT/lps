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
    SYS(brk, sys_brk(t, a0))
    SYS(mmap, sys_mmap(t, a0, a1, a2, a3, a4, a5))
    SYS(mprotect, sys_mprotect(t, a0, a1, a2))
    SYS(munmap, sys_munmap(t, a0, a1))
    SYS(clone, sys_nosys(t, "clone"))
    SYS(exit_group, sys_exit_group(t, a0))
    SYS(exit, sys_exit(t, a0))
    LPS(runtime_call, sys_runtime_call(t, a0));
    }

    if (!handled && t->proc->engine->opts.passthrough) {
        r = sys_passthrough(t, sysno, a0, a1, a2, a3, a4, a5);
        return r;
    }

    if (handled) return r;

    switch (sysno) {
    SYS(getpid, 0)
    SYS(write, sys_write(t, a0, a1, a2))
    SYS(writev, sys_writev(t, a0, a1, a2))
    SYS(read, sys_read(t, a0, a1, a2))
    SYS(readv, sys_readv(t, a0, a1, a2))
    SYS(openat, sys_openat(t, a0, a1, a2, a3))
    SYS(close, sys_close(t, a0))

    // Thread support calls.
    SYS(gettid, sys_gettid(t))
    SYS(futex, sys_nosys(t, "futex"))
    SYS(set_tid_address, sys_set_tid_address(t, a0))
    SYS(sched_getaffinity, sys_nosys(t, "sched_getaffinity"))
    SYS(sched_setaffinity, sys_nosys(t, "sched_setaffinity"))
    SYS(sched_yield, sys_nosys(t, "sched_yield"))

    // Signals (also needed, at least as stubs, for threads).
    SYS(rt_sigaction, sys_ignore(t, "rt_sigaction"))
    SYS(rt_sigprocmask, sys_ignore(t, "rt_sigprocmask"))
    SYS(rt_sigreturn, sys_ignore(t, "rt_sigreturn"))
    SYS(sigaltstack, sys_ignore(t, "sigaltstack"))

    // File operations.
    SYS(lseek, sys_lseek(t, a0, a1, a2))
    SYS(pread64, sys_pread64(t, a0, a1, a2, a3))
    SYS(getdents64, sys_getdents64(t, a0, a1, a2))
    SYS(newfstatat, sys_newfstatat(t, a0, a1, a2, a3))
    SYS(fstat, sys_newfstatat(t, a0, 0, a1, LINUX_AT_EMPTY_PATH))
    SYS(fchmod, sys_fchmod(t, a0, a1))
    SYS(truncate, sys_truncate(t, a0, a1))
    SYS(ftruncate, sys_ftruncate(t, a0, a1))
    SYS(fchown, sys_fchown(t, a0, a1, a2))
    SYS(fsync, sys_fsync(t, a0))
    SYS(mkdirat, sys_mkdirat(t, a0, a1, a2))
    SYS(unlinkat, sys_unlinkat(t, a0, a1, a2))
    SYS(renameat2, sys_renameat2(t, a0, a1, a2, a3, a4))
    SYS(faccessat, sys_faccessat(t, a0, a1, a2))
    SYS(readlinkat, sys_readlinkat(t, a0, a1, a2, a3))
    SYS(dup, sys_dup(t, a0))
    SYS(dup3, sys_dup3(t, a0, a1, a2))

    // Working directory syscalls.
    SYS(chdir, sys_chdir(t, a0))
    SYS(fchdir, sys_fchdir(t, a0))
    SYS(getcwd, sys_getcwd(t, a0, a1))

    // Time syscalls.
    SYS(nanosleep, sys_nanosleep(t, a0, a1))
    SYS(clock_gettime, sys_clock_gettime(t, a0, a1))
    
    // Other syscalls.
    SYS(getrandom, sys_getrandom(t, a0, a1, a2))
    SYS(ioctl, sys_ioctl(t, a0, a1, a2, a3, a4, a5))
    SYS(fcntl, sys_ignore(t, "fcntl"))
    SYS(prctl, sys_prctl(t, a0, a1, a2, a3, a4))
    SYS(uname, sys_uname(t, a0))
    SYS(sysinfo, sys_sysinfo(t, a0))
    SYS(getrlimit, sys_getrlimit(t, a0, a1))

    // Unsupported syscalls that we ignore or purposefully return ENOSYS for.
    SYS(set_robust_list, sys_ignore(t, "set_robust_list"))
    SYS(membarrier, sys_ignore(t, "membarrier"))
    SYS(madvise, sys_ignore(t, "madvise"))
    SYS(getrusage, sys_ignore(t, "getrusage"))
    SYS(statx, sys_nosys(t, "statx"))
    SYS(rseq, sys_nosys(t, "rseq"))
    SYS(prlimit64, sys_nosys(t, "prlimit64"))
    SYS(statfs, sys_nosys(t, "statfs"))
    SYS(getxattr, sys_nosys(t, "getxattr"))
    SYS(lgetxattr, sys_nosys(t, "lgetxattr"))
    SYS(socket, sys_nosys(t, "socket"))
    SYS(mremap, sys_nosys(t, "mremap"))
    SYS(utimensat, sys_nosys(t, "utimensat"))
    
    default:
        LOG(t->proc->engine, "unknown syscall: %ld", sysno);
        ERROR("terminating due to unknown syscall: %ld", sysno);
        exit(1);
    } 

    return r;
}

inline static uint64_t get_time(void) {
    uint64_t time;
    asm volatile ("rdtime %0" : "=r"(time));
    return time;
}

void arch_syshandle(struct LPSContext *ctx)
{
    struct LPSThread *t = lps_ctx_data(ctx);
    assert(t);
    struct LPSRegs *regs = lps_ctx_regs(ctx);

    if (regs->ucasue == CAUSE_IRQ_U_TIME) {
        // TODO: check overflow
        uint64_t next = get_time() + 10000000;
		DBG("[U_INTR_HANDLER] set utimecmp to %lu", next);
		csr_write(CSR_UTIMECMP, next);

        // TODO: sched
        lps_thread_exit(t);
        return;
    }

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
        assert(0);
    }

    // uepc += 4 TODO: 2字节指令对齐支持
    regs->uepc += 4;

    regs->a0 = syshandle(t, regs->a7, regs->a0, regs->a1, regs->a2, regs->a3,
        regs->a4, regs->a5);
}
