// test_cross.c - Cross-sandbox isolation test
//
// Proves that sandbox A cannot read memory belonging to sandbox B.
// argv[2] points to a mapped page inside box_b. Attempting to load
// from it triggers a DASICS load fault (the address is mapped but
// outside box_a's DASICS memory bounds).

#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc < 3) return 1;

    uintptr_t cross_target = (uintptr_t)strtoul(argv[2], NULL, 16);

    // Load from box_b's memory — outside box_a's DASICS bounds.
    volatile uint64_t val;
    __asm__ volatile("ld %0, 0(%1)" : "=r"(val) : "r"(cross_target));

    // Should never reach here.
    return 1;
}
