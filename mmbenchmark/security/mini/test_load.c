// test_load.c - DASICS load fault security test
//
// Attempts to read from an address outside the sandbox's DASICS memory
// bounds. The target address (passed via argv[1]) points to a host-mapped
// page, ensuring a DASICS load fault (DFR_LF) instead of a page fault.
//
// Expected: DASICS intercepts the ld instruction and raises a load fault.

#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    uintptr_t target = (uintptr_t)strtoul(argv[1], NULL, 16);

    // This ld instruction targets an address outside DASICS membound.
    // DASICS hardware will check the address against configured bounds
    // and raise a load fault (DFR_LF = 2) before the load completes.
    volatile uint64_t val;
    __asm__ volatile(
        "ld %0, 0(%1)"
        : "=r"(val)
        : "r"(target)
    );

    // Should never reach here - DASICS fault occurs at the ld above
    return 1;
}
