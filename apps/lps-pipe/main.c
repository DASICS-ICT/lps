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


#define PIPE_READ_FD  3  // 从pong读取 (从ping读取)
#define PIPE_WRITE_FD 4  // 向pong写入 (向ping写入)

// usage: pipe-test [ping] [pong]
int
main(int argc, char** argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [ping] [pong]\n", argv[0]);
        return 1;
    }

    struct LPSEngine *lps = lps_new((struct LPSOptions) {
        .verbose = true,
        .boxsize = gb(4),
    }, 10);
    assert(lps);

    struct LPSLinuxEngine *lps_linux = lps_linux_new(lps, (struct LPSLinuxOpts) {
        .verbose = true,
        .passthrough = false,
    });
    assert(lps_linux);

    struct LPSProc *ping_proc = lps_proc_new(lps_linux);
    assert(ping_proc);
    struct LPSProc *pong_proc = lps_proc_new(lps_linux);
    assert(pong_proc);

    const char *ping_path = argv[1];
    const char *pong_path = argv[2];

    struct Buf ping_elf = readfile(ping_path);
    assert(ping_elf.data);
    struct Buf pong_elf = readfile(pong_path);
    assert(pong_elf.data);
    
    assert(lps_proc_load(ping_proc, ping_elf.data, ping_elf.size, ping_path));
    assert(lps_proc_load(pong_proc, pong_elf.data, pong_elf.size, pong_path));

    // pipe: ping to pong
    assert(lps_pipe_new(ping_proc, PIPE_WRITE_FD, pong_proc, PIPE_READ_FD));
    // pipe: pong to ping
    assert(lps_pipe_new(pong_proc, PIPE_WRITE_FD, ping_proc, PIPE_READ_FD));

    const char *envp = NULL;
    struct LPSThread *ping = lps_thread_new(ping_proc, 0, NULL, &envp);
    struct LPSThread *pong = lps_thread_new(pong_proc, 0, NULL, &envp);

    rrschedadd(ping);
    rrschedadd(pong);

    rrschedstart(false);
    
    return 0;
}