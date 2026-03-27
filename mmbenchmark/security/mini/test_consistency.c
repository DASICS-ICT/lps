// test_consistency.c - DASICS state consistency after fault recovery
//
// Verifies that DASICS bounds are not corrupted after the exception
// handler processes a fault and resumes sandbox execution.
//
// Flow:
//   1. ecall write "step1" → proves normal ecall works
//   2. ld from bait with SKIP_MAGIC → handler skips, sets a0 = -1
//   3. ecall write "step2" → proves DASICS state survived the fault
//
// If step 3 output appears, the entire exception path
// (SAVE_ALL → handler → RESTORE_ALL → URET) preserved DASICS state.

#include <stdlib.h>
#include <stdint.h>
#include "test_common.h"

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    uintptr_t bait = (uintptr_t)strtoul(argv[1], NULL, 16);

    // Step 1: normal syscall.
    print("  step1: ecall before fault: OK\n");

    // Step 2: deliberate DASICS fault with skip-and-continue.
    long ret;
    __asm__ volatile(
        "li a6, %2\n\t"
        "slli a6, a6, 32\n\t"
        "li t0, %2\n\t"
        "or a6, a6, t0\n\t"   // a6 = SKIP_MAGIC (64-bit)
        "li a0, 0\n\t"
        "ld t0, 0(%1)\n\t"    // fault here → handler skips
        "mv %0, a0\n\t"
        : "=r"(ret)
        : "r"(bait), "i"((int)(SKIP_MAGIC & 0xFFFFFFFF))
        : "a0", "a6", "t0", "memory"
    );

    if (ret != -1) {
        print("FAIL [consistency]: fault was not caught\n");
        raw_exit(1);
    }

    // Step 3: normal syscall after fault recovery.
    // If DASICS state was corrupted, this ecall would either
    // fault or be mishandled.
    print("PASS [consistency]: ecall after fault recovery: OK\n");
    raw_exit(0);
}
