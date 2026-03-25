#include "lps_linux.h"

#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>

static size_t
gb(size_t x)
{
    return x * 1024 * 1024 * 1024;
}

static size_t
mb(size_t x)
{
    return x * 1024 * 1024;
}

static size_t
kb(size_t x)
{
    return x * 1024;
}

int
main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [ELF]\n", argv[0]);
        return 1;
    }

    struct LPSEngine *engine = lps_new((struct LPSOptions) {
        .boxsize = gb(4),
        .verbose = false,
    }, 10);
    assert(engine != NULL);

    struct LPSLinuxEngine *x_engine = lps_linux_new(engine, (struct LPSLinuxOpts) {
        .verbose = false,
        .passthrough = false,
    });
    assert(x_engine != NULL);

    struct LPSProc *proc = lps_proc_new(x_engine);
    assert(proc != NULL);

    const char *elf_path = argv[1];

    bool ok = lps_proc_load_file(proc, elf_path);
    assert(ok == true);

    const char *envp = NULL;

    struct LPSThread *t = lps_thread_new(proc, argc - 1, (const char **)(argv + 1) , &envp);

    rrschedadd(t);
    rrschedstart(false);
    
    return 0;
}