// test_store.c - DASICS store fault security test
//
// Attempts to write to an address outside the sandbox's DASICS memory
// bounds. The target address (passed via argv[1]) points to a host-mapped
// page, ensuring a DASICS store fault (DFR_SF) instead of a page fault.
//
// Expected: DASICS intercepts the sd instruction and raises a store fault.

#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    uintptr_t target = (uintptr_t)strtoul(argv[1], NULL, 16);

    // This sd instruction targets an address outside DASICS membound.
    // DASICS hardware will check the address against configured bounds
    // and raise a store fault (DFR_SF = 3) before the store completes.
    uint64_t val = 0xdeadbeefcafebabe;
    __asm__ volatile(
        "sd %0, 0(%1)"
        :
        : "r"(val), "r"(target)
        : "memory"
    );

    // Should never reach here - DASICS fault occurs at the sd above
    return 1;
}
