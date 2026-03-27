// test_boundary.c - DASICS boundary precision test
//
// Verifies that the DASICS bound is exact: box_end-8 is accessible
// (last 8 bytes of the box / stack), while box_end triggers a fault.
//
// argv[3] = box_end (= box_b_base, page is mapped in box_b).
// The stack occupies [box_end - stacksize, box_end), so box_end-8
// is the last valid qword.

#include <stdlib.h>
#include <stdint.h>
#include "test_common.h"

int main(int argc, char **argv) {
    if (argc < 4) return 1;
    uintptr_t box_end = (uintptr_t)strtoul(argv[3], NULL, 16);

    // Step 1: access last 8 bytes inside box (should succeed).
    volatile uint64_t val;
    __asm__ volatile("ld %0, 0(%1)" : "=r"(val) : "r"(box_end - 8));

    // If we get here, in-bound access succeeded.
    print("  in-bound access at box_end-8: OK\n");

    // Step 2: access first byte outside box (should trigger DFR_LF).
    // box_end is box_b_base with a mapped page, so DASICS catches it
    // instead of a page fault.
    __asm__ volatile("ld %0, 0(%1)" : "=r"(val) : "r"(box_end));

    // Should never reach here.
    print("FAIL [boundary]: out-of-bound access at box_end succeeded!\n");
    raw_exit(1);
}
