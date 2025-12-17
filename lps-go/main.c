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

struct Buf {
    void *data;
    size_t size;
};

static struct Buf
readfile(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        return (struct Buf) { 0 };
    }
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    void *p = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fileno(f), 0);
    fclose(f);
    if (!p) {
        return (struct Buf) { 0 };
    }
    return (struct Buf) {
        .data = p,
        .size = sz,
    };
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
        .verbose = true,
    }, 10);
    assert(engine != NULL);

    struct LPSLinuxEngine *x_engine = lps_linux_new(engine, (struct LPSLinuxOpts) {
        .verbose = true,
        .passthrough = false,
    });
    assert(x_engine != NULL);

    struct LPSProc *proc = lps_proc_new(x_engine);
    assert(proc != NULL);

    const char *elf_path = argv[1];

    struct Buf prog = readfile(elf_path);
    assert(prog.data != NULL);

    bool ok = lps_proc_load(proc, prog.data, prog.size, NULL);
    assert(ok == true);

    const char *envp = NULL;

    struct LPSThread *t = lps_thread_new(proc, argc - 1, (const char **)(argv + 1) , &envp);

    rrschedadd(t);
    rrschedstart();
    
    return 0;
}