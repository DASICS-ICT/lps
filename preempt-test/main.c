#include <stdio.h>
#include <assert.h>

#include "lfi_tux.h"
#include "lfi.h"

typedef struct {
    uint8_t* data;
    size_t size;
} buf_t;

buf_t bufreadfile(struct Tux* tux, const char* filename);

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


// usage: preempt-test [ping] [pong]
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

    scheduler_add_task(ping);
    scheduler_add_task(pong);

    scheduler_begin();
    
    return 0;
}