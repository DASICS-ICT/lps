#include <stdio.h>
#include <assert.h>

#include "lfi_tux.h"
#include "lfi.h"

typedef struct {
    uint8_t* data;
    size_t size;
} buf_t;

buf_t bufreadfile(struct Tux* tux, const char* filename);
bool pipe_new(struct FDFile **f0, struct FDFile **f1);

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

    const char* ping_path = argv[1];
    const char* pong_path = argv[2];

    struct LFIPlatform* plat = lfi_new_plat((struct LFIPlatOptions) {
        .pagesize = kb(4),
        .vmsize = gb(4),
        .poc = false,
    });

    struct Tux* tux = lfi_tux_new(plat, (struct TuxOptions) {
        .pagesize = kb(4),
        .verbose = true,
        .stacksize = mb(2),
    });

    buf_t ping_elf = bufreadfile(tux, ping_path);
    buf_t pong_elf = bufreadfile(tux, pong_path);

    if (!ping_elf.data || !pong_elf.data) {
        fprintf(stderr, "error opening: %s or %s\n", ping_path, pong_path);
        return 1;
    }

    struct TuxThread* ping = lfi_tux_proc_new(tux, ping_elf.data, ping_elf.size, 0, NULL);

    struct TuxThread* pong = lfi_tux_proc_new(tux, pong_elf.data, pong_elf.size, 0, NULL);

    struct FDFile *f0, *f1, *f2, *f3;
    // pipe(f0, f1) f1 -> f0
    // pipe(f2, f3) f3 -> f2

    assert(pipe_new(&f0, &f1));
    assert(pipe_new(&f2, &f3));

    proc_fd_assign(ping, PIPE_READ_FD, f0);
    proc_fd_assign(ping, PIPE_WRITE_FD, f3);

    proc_fd_assign(pong, PIPE_READ_FD, f2);
    proc_fd_assign(pong, PIPE_WRITE_FD, f1);

    scheduler_add_task(ping);
    scheduler_add_task(pong);

    scheduler_begin();
    
    return 0;
}