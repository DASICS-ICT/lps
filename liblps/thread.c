#include "linux.h"
#include "proc.h"
#include "align.h"
#include "lps_arch.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/random.h>

// First TID, to avoid using low TID numbers.
#define BASE_TID 10000
// Maximum number of argv arguments.
#define ARGV_MAX 1024
// Maxmimum number of envp arguments.
#define ENVP_MAX 1024
// Maximum length of an argv string.
#define ARGV_MAXLEN 1024
// Maxmimum length of an envp string.
#define ENVP_MAXLEN 1024

#include <sys/auxv.h>
static unsigned long
host_getauxval(unsigned long type)
{
    switch (type) {
    case LINUX_AT_HWCAP:
        return getauxval(AT_HWCAP);
    case LINUX_AT_HWCAP2:
        return getauxval(AT_HWCAP2);
    }
    return 0;
}

static int
next_tid(struct LPSProc *p)
{
    return BASE_TID +
        atomic_fetch_add_explicit(&p->total_thread_count, 1,
            memory_order_relaxed);
}

struct Auxv {
    uint64_t a_type;
    uint64_t a_val;
};

struct AuxvList {
    struct Auxv at_secure;
    struct Auxv at_base;
    struct Auxv at_phdr;
    struct Auxv at_phnum;
    struct Auxv at_phent;
    struct Auxv at_entry;
    struct Auxv at_execfn;
    struct Auxv at_pagesz;
    struct Auxv at_hwcap;
    struct Auxv at_hwcap2;
    struct Auxv at_random;
    struct Auxv at_flags;
    struct Auxv at_uid;
    struct Auxv at_euid;
    struct Auxv at_gid;
    struct Auxv at_egid;
    struct Auxv at_sysinfo;
    struct Auxv at_sysinfo_ehdr;
    struct Auxv at_null;
};

// Initializes the thread's stack with argv, envp, and auxv.
//
// The layout will look something like this:
// | argc | argv[0] .. 0 | envp[0] .. 0 | auxv | rand | argv/envp strings |
// ^ sp
static uintptr_t
stack_init(struct LPSThread *t, int argc, const char **argv,
    const char **envp)
{
    argc = MIN(argc, ARGV_MAX);

    // Calculate how many bytes we will need to copy from argv and envp.
    size_t strs_len = 0;
    size_t nargv = 0;
    for (int i = 0; i < argc; i++) {
        if (!argv[i])
            break;
        size_t len = strnlen(argv[i], ARGV_MAXLEN);
        assert(argv[i][len] == '\0');
        strs_len += len + 1;
        nargv++;
    }
    size_t nenvp = 0;
    for (int i = 0; i < ENVP_MAX; i++) {
        if (!envp[i])
            break;
        size_t len = strnlen(envp[i], ENVP_MAXLEN);
        assert(envp[i][len] == '\0');
        strs_len += len + 1;
        nenvp++;
    }

    // Store the sandbox pointers to each argv/envp string.
    uintptr_t box_argv[nargv + 1];
    uintptr_t box_envp[nenvp + 1];
    box_argv[nargv] = 0;
    box_envp[nenvp] = 0;

    // We make this 16-byte aligned so that we can use this as a base offset
    // for also storing aligned values.
    uintptr_t strs_start = truncp(t->stack + t->stack_size - strs_len, 16);
    size_t count = 0;
    // Copy the argv and envp strings into the sandbox.
    for (size_t i = 0; i < nargv; i++) {
        size_t len = strnlen(argv[i], ARGV_MAXLEN) + 1;
        box_argv[i] = (uintptr_t) memcpy((void *) (strs_start + count), argv[i], len);
        // LOG(t->proc->engine, "stack_init: copy (%s) on 0x%lx", argv[i], (strs_start + count));
        count += len;
    }
    for (size_t i = 0; i < nenvp; i++) {
        size_t len = strnlen(envp[i], ARGV_MAXLEN) + 1;
        box_envp[i] = (uintptr_t) memcpy((void *) (strs_start + count), envp[i], len);
        // LOG(t->proc->engine, "stack_init: copy (%s) on 0x%lx", envp[i], (strs_start + count));
        count += len;
    }

    // Create 16 random bytes for AT_RANDOM.
    char random[16];
    ssize_t r = getrandom(&random[0], sizeof(random), 0);
    if (r != sizeof(random)) {
        LOG(t->proc->engine,
            "warning: getrandom for AT_RANDOM returned %ld (less than 16)", r);
    }
    uintptr_t rand_start = strs_start - sizeof(random);
    // Copy into the sandbox.
    memcpy((void *) rand_start, &random[0], sizeof(random));
    // LOG(t->proc->engine, "stack_init: copy random(%luB) on 0x%lx", sizeof(random), rand_start);

    // Create the auxiliary vector.
    struct AuxvList auxv = {
        (struct Auxv) { LINUX_AT_SECURE, 0 },
        (struct Auxv) { LINUX_AT_BASE, t->proc->elfinfo.ldbase },
        (struct Auxv) { LINUX_AT_PHDR,
            t->proc->elfinfo.elfbase + t->proc->elfinfo.elfphoff },
        (struct Auxv) { LINUX_AT_PHNUM, t->proc->elfinfo.elfphnum },
        (struct Auxv) { LINUX_AT_PHENT, t->proc->elfinfo.elfphentsize },
        (struct Auxv) { LINUX_AT_ENTRY, t->proc->elfinfo.elfentry },
        (struct Auxv) { LINUX_AT_EXECFN, box_argv[0] },
        (struct Auxv) { LINUX_AT_PAGESZ, 4 * 1024},
        (struct Auxv) { LINUX_AT_HWCAP, host_getauxval(LINUX_AT_HWCAP) },
        (struct Auxv) { LINUX_AT_HWCAP2, host_getauxval(LINUX_AT_HWCAP2) },
        (struct Auxv) { LINUX_AT_RANDOM, rand_start },
        (struct Auxv) { LINUX_AT_FLAGS, 0 },
        (struct Auxv) { LINUX_AT_UID, 1000 },
        (struct Auxv) { LINUX_AT_EUID, 1000 },
        (struct Auxv) { LINUX_AT_GID, 1000 },
        (struct Auxv) { LINUX_AT_EGID, 1000 },
        (struct Auxv) { LINUX_AT_SYSINFO, 0 },
        (struct Auxv) { LINUX_AT_SYSINFO_EHDR, 0 },
        (struct Auxv) { LINUX_AT_NULL, 0 },
    };

    uint64_t box_argc = argc;
    // Calculate where the sp should go (enough space from rand_start to store
    // argc/argv/envp/auxv).
    uintptr_t stack_start = rand_start - sizeof(box_argc) - sizeof(box_argv) -
        sizeof(box_envp) - sizeof(auxv);
    // Copy each item onto the stack.
    // LOG(t->proc->engine, "stack_init: copy argc(%lu) on 0x%lx", box_argc, stack_start);
    uintptr_t next = (uintptr_t) memcpy((void *) stack_start, &box_argc,
                      sizeof(box_argc)) +
        sizeof(box_argc);
    // LOG(t->proc->engine, "stack_init: copy argv(%luB) on 0x%lx", sizeof(box_argv), next);
    next = (uintptr_t) memcpy((void *) next, box_argv, sizeof(box_argv)) +
        sizeof(box_argv);
    // LOG(t->proc->engine, "stack_init: copy envp(%luB) on 0x%lx", sizeof(box_envp), next);
    next = (uintptr_t) memcpy((void *) next, box_envp, sizeof(box_envp)) +
        sizeof(box_envp);
    // LOG(t->proc->engine, "stack_init: copy auxv(%luB) on 0x%lx", sizeof(auxv), next);
    memcpy((void *) next, &auxv, sizeof(auxv));

    return stack_start;
}

static void
sp_init(struct LPSThread *t, uintptr_t sp) 
{
    struct LPSRegs *regs = lps_ctx_regs(t->ctx);
    regs->sp = sp;
    LOG(t->proc->engine, "sp_init: 0x%lx", sp);
}

static void
uepc_init(struct LPSThread *t, uintptr_t uepc) {
    struct LPSRegs *regs = lps_ctx_regs(t->ctx);
    regs->uepc = uepc;
    LOG(t->proc->engine, "uepc_init: 0x%lx", uepc);
}

struct LPSThread * 
lps_thread_new(struct LPSProc *proc, int argc, const char **argv,
    const char **envp)
{
    struct LPSThread *t = calloc(sizeof(struct LPSThread), 1);
    if (!t) {
        goto err1;
    }
    t->proc = proc;
    t->ctx = lps_ctx_new(proc->box, t);
    if (!t->ctx) {
        goto err2;
    }
    t->tid = next_tid(proc);

    size_t stacksize = 2ULL * 1024 * 1024; // TODO: configurable
    uintptr_t end = proc->boxinfo.base + proc->boxinfo.size;
    t->stack = lps_box_mapat(proc->box, end - stacksize, stacksize, 
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (t->stack == (uintptr_t) -1) {
        goto err3;
    }
    t->stack_size = stacksize;
    LOG(proc->engine, "map stack at [0x%lx, 0x%lx]", t->stack, end);
    
    uintptr_t sp = stack_init(t, argc, argv, envp);
    sp_init(t, sp);
    uepc_init(t, proc->entry);

    return t;
err3:
    lps_ctx_free(t->ctx);
err2:
    free(t);
err1:
    return NULL;
}

EXPORT void
lps_thread_run(struct LPSThread *t)
{
    LOG(t->proc->engine, "Thread %lx entered", t);
    lps_kswitch_to(t->ctx);
}

EXPORT void
lps_thread_exit(struct LPSThread *t)
{
    LOG(t->proc->engine, "Thread %lx exited", t);
    lps_kswitch_from(t->ctx);
}