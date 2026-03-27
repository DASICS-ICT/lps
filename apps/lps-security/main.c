// lps-security: DASICS security micro-test runner
//
// Runs a sandbox ELF and catches DASICS violations gracefully.
// Allocates mapped pages outside the sandbox box so that access
// triggers DASICS faults instead of page faults.
//
// Sandbox argv layout:
//   argv[0] = elf_path
//   argv[1] = host bait page address     (host mmap, outside box)
//   argv[2] = cross-sandbox page address  (mapped in box_b)
//   argv[3] = box_a end address           (= box_b base)
//
// Usage: lps-security <test-elf> [-dasics]

#include "lps_linux.h"
#include "lps_arch.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>

// Forward declare lps-linux internal syshandle (linked via liblps-linux.a)
extern uintptr_t
syshandle(struct LPSThread *t, uintptr_t sysno, uintptr_t a0, uintptr_t a1,
    uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5);

#define CAUSE_DASICS_U_CHECK_FAULT 0x18
#define DFR_EF  1
#define DFR_LF  2
#define DFR_SF  3
#define DFR_JF  4

// Marker syscall number for test_ecall (not a real Linux syscall).
#define ECALL_TEST_MARKER 9999

// Magic values in a6 for handler protocol.
// Sandbox sets a6 before a deliberate fault to tell the handler
// to skip the instruction and continue instead of terminating.
#define SKIP_MAGIC 0x5B1D5B1D5B1D5B1DULL
#define CSR_MAGIC  0xC5B0C5B0C5B0C5B0ULL

// ---- SIGILL handling for CSR tamper test ----
// csrw to DASICS CSRs from sandboxed code triggers Illegal Instruction
// (kernel-handled, not routed to UTVEC). The kernel sends SIGILL.
// We catch it here so the test reports PASS instead of crashing.
static sigjmp_buf sigill_jmp;

static void
sigill_handler(int sig, siginfo_t *info, void *uctx)
{
    (void)info; (void)uctx;
    siglongjmp(sigill_jmp, sig);
}

static void
sigsegv_handler(int sig, siginfo_t *info, void *uctx)
{
    (void)uctx;
    fprintf(stderr, "SEGFAULT: addr=%p, si_code=%d\n",
            info->si_addr, info->si_code);
    siglongjmp(sigill_jmp, sig);
}

static void
setup_signal_handlers(void)
{
    // Use alternate signal stack so handlers work even when
    // the CPU sp points into the sandbox (not the host stack).
    static char sigstack[SIGSTKSZ];
    stack_t ss = { .ss_sp = sigstack, .ss_size = SIGSTKSZ };
    sigaltstack(&ss, NULL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigill_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGILL, &sa, NULL);

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigsegv_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGSEGV, &sa, NULL);
}

static size_t
gb(size_t x)
{
    return x * 1024 * 1024 * 1024;
}

static void
security_syshandle(struct LPSContext *ctx)
{
    struct LPSThread *t = (struct LPSThread *)lps_ctx_data(ctx);
    struct LPSRegs *regs = lps_ctx_regs(ctx);

    // ---- Non-DASICS fault (e.g. illegal instruction from csrw) ----
    if (regs->ucause != CAUSE_DASICS_U_CHECK_FAULT) {
        if (regs->a6 == CSR_MAGIC) {
            printf("PASS [csr_tamper]: CSR write blocked (ucause=0x%lx)\n"
                   "  uepc   = 0x%lx\n",
                   regs->ucause, regs->uepc);
            regs->uepc += 4;
            regs->a0 = (uintptr_t)-1;
            return;
        }
        fprintf(stderr, "FAIL: unexpected ucause=0x%lx (expected 0x18)\n",
                regs->ucause);
        lps_thread_exit(t);
        return;
    }

    // ---- DASICS fault ----

    // Skip-and-continue: sandbox deliberately triggered a fault
    // and wants to resume execution after it.
    if (regs->dfreason != DFR_EF && regs->a6 == SKIP_MAGIC) {
        regs->uepc += 4;
        regs->a0 = (uintptr_t)-1;
        return;
    }

    // CSR tamper: DASICS caught a CSR write attempt
    if (regs->dfreason != DFR_EF && regs->a6 == CSR_MAGIC) {
        printf("PASS [csr_tamper]: DASICS blocked CSR access (dfreason=%lu)\n"
               "  uepc   = 0x%lx\n",
               regs->dfreason, regs->uepc);
        regs->uepc += 4;
        regs->a0 = (uintptr_t)-1;
        return;
    }

    // ---- Normal DASICS fault handling ----
    switch (regs->dfreason) {
    case DFR_EF:
        regs->uepc += 4;
        if (regs->a7 == ECALL_TEST_MARKER) {
            printf("PASS [ecall]: DASICS intercepted ecall (DFR_EF)\n"
                   "  uepc   = 0x%lx\n"
                   "  sysno  = %lu (test marker, not a real Linux syscall)\n",
                   regs->uepc - 4, regs->a7);
            regs->a0 = 0;
            return;
        }
        regs->a0 = syshandle(t, regs->a7, regs->a0, regs->a1,
            regs->a2, regs->a3, regs->a4, regs->a5);
        return;

    case DFR_LF:
        printf("PASS [load_fault]: DASICS blocked out-of-bound load\n"
               "  uepc   = 0x%lx\n"
               "  addr   = 0x%lx\n",
               regs->uepc, regs->utval);
        break;

    case DFR_SF:
        printf("PASS [store_fault]: DASICS blocked out-of-bound store\n"
               "  uepc   = 0x%lx\n"
               "  addr   = 0x%lx\n",
               regs->uepc, regs->utval);
        break;

    case DFR_JF:
        printf("PASS [jump_fault]: DASICS blocked out-of-bound jump/return\n"
               "  uepc   = 0x%lx\n"
               "  target = 0x%lx\n",
               regs->uepc, regs->utval);
        break;

    default:
        fprintf(stderr, "FAIL: unknown dfreason=%lu\n", regs->dfreason);
        break;
    }

    lps_thread_exit(t);
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,
            "Usage: %s <test-elf> [-dasics]\n"
            "\n"
            "DASICS security micro-test runner.\n"
            "Sandbox argv: [elf, host_bait, cross_bait, box_end]\n",
            argv[0]);
        return 1;
    }

    // Create LPS engine
    struct LPSEngine *engine = lps_new((struct LPSOptions) {
        .boxsize = gb(4),
        .verbose = false,
    }, 10);
    assert(engine);

    struct LPSLinuxEngine *x_engine = lps_linux_new(engine, (struct LPSLinuxOpts) {
        .verbose = false,
        .passthrough = false,
    });
    assert(x_engine);

    // Override default handler with our security-aware version
    lps_sys_handler(engine, security_syshandle);

    // Create proc (allocates box_a from boxmap)
    struct LPSProc *proc = lps_proc_new(x_engine);
    assert(proc);

    const char *elf_path = argv[1];
    bool ok = lps_proc_load_file(proc, elf_path);
    if (!ok) {
        fprintf(stderr, "Failed to load ELF: %s\n", elf_path);
        return 1;
    }

    // ---- Bait page 1: host-mapped page (outside all boxes) ----
    void *bait = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(bait != MAP_FAILED);
    memset(bait, 0x42, 4096);

    // ---- Bait page 2: cross-sandbox page in box_b ----
    // boxmap allocates sequentially, so box_b_base == box_a_end.
    struct LPSBox *box_b = lps_box_new(engine);
    assert(box_b);
    struct LPSBoxInfo bi_b = lps_box_info(box_b);
    uintptr_t cross_page = lps_box_mapat(box_b, bi_b.base, 4096,
        PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(cross_page != (uintptr_t)-1);
    memset((void *)cross_page, 0xAB, 4096);

    // box_a_end == box_b_base (sequential allocation)
    uintptr_t box_a_end = bi_b.base;

    // ---- Build sandbox argv ----
    char bait_str[32], cross_str[32], end_str[32];
    snprintf(bait_str,  sizeof(bait_str),  "0x%lx", (unsigned long)bait);
    snprintf(cross_str, sizeof(cross_str), "0x%lx", (unsigned long)cross_page);
    snprintf(end_str,   sizeof(end_str),   "0x%lx", (unsigned long)box_a_end);

    const char *sandbox_argv[] = { elf_path, bait_str, cross_str, end_str, NULL };
    const char *envp = NULL;

    struct LPSThread *t = lps_thread_new(proc, 4, sandbox_argv, &envp);
    assert(t);

    // Install signal handlers:
    //   SIGILL  - csrw to DASICS CSRs → Illegal Instruction
    //   SIGSEGV - access to unmapped page (helps diagnose bait page issues)
    setup_signal_handlers();

    int sig = sigsetjmp(sigill_jmp, 1);
    if (sig == 0) {
        lps_thread_run(t);
    } else if (sig == SIGILL) {
        printf("PASS [csr_tamper]: CSR write blocked (SIGILL from kernel)\n");
    } else if (sig == SIGSEGV) {
        fprintf(stderr, "FAIL: segmentation fault in sandbox "
                "(target page not mapped? check bait setup)\n");
    }

    munmap(bait, 4096);
    return 0;
}
