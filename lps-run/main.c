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


int
main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [ELF]\n", argv[0]);
        return 1;
    }

    const char* elf_path = argv[1];

    struct LFIPlatform* plat = lfi_new_plat((struct LFIPlatOptions) {
        .pagesize = kb(4),
        .vmsize = gb(4),
        .poc = false,
    });

    struct Tux* tux = lfi_tux_new(plat, (struct TuxOptions) {
        .pagesize = kb(4),
        .verbose = false,
        .stacksize = mb(2),
    });

    buf_t elf = bufreadfile(tux, elf_path);
    if (!elf.data) {
        fprintf(stderr, "error opening: %s\n", elf_path);
        return 1;
    }

    struct TuxThread* p = lfi_tux_proc_new(tux, elf.data, elf.size, argc - 1, argv + 1);

    scheduler_add_task(p);

    scheduler_begin();
    
    return 0;
}