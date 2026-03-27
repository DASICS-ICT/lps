// test_mmap.c - mmap escape test
//
// Verifies that the sandbox cannot extend its address space outside
// the box via mmap(MAP_FIXED).  sys_mmap checks ptrcheck(addr)
// before mapping, so an out-of-box address should return -1.

#include <stdlib.h>
#include <stdint.h>
#include "test_common.h"

// Linux mmap flags
#define MAP_PRIVATE   2
#define MAP_FIXED     16
#define MAP_ANONYMOUS 32
#define PROT_READ     1
#define PROT_WRITE    2

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    uintptr_t bait = (uintptr_t)strtoul(argv[1], NULL, 16);

    // Round bait to page boundary.
    uintptr_t page = bait & ~0xFFFUL;

    // Try to mmap a page at a host address (outside box).
    long ret = raw_mmap(page, 4096,
                        PROT_READ | PROT_WRITE,
                        MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE,
                        -1, 0);

    if (ret == -1 || ret < 0) {
        print("PASS [mmap_escape]: mmap(MAP_FIXED) outside box rejected\n");
        raw_exit(0);
    }

    print("FAIL [mmap_escape]: mmap(MAP_FIXED) outside box succeeded!\n");
    raw_exit(1);
}
