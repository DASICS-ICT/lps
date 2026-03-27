// test_csr.c - DASICS CSR tampering defense test
//
// Attempts to escape the sandbox by writing to DASICS bound CSRs.
// On DASICS hardware, csrw to DLCFG from sandboxed code triggers
// an Illegal Instruction exception (handled by the kernel, not the
// user-mode DASICS handler). The runner catches the resulting SIGILL
// and reports PASS.
//
// Fallback: if csrw somehow succeeds, an out-of-bound load verifies
// whether the bounds were actually disabled.

#include <stdlib.h>
#include <stdint.h>
#include "test_common.h"

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    uintptr_t bait = (uintptr_t)strtoul(argv[1], NULL, 16);

    // Step 1: try to clear DLCFG (disable all memory bounds).
    // Expected: Illegal Instruction → kernel SIGILL → runner catches → PASS
    __asm__ volatile("csrw 0x880, zero" ::: "memory");

    // Step 2: csrw didn't crash. Verify bounds are still active.
    // OOB load should trigger DASICS DFR_LF → handler prints PASS.
    volatile uint64_t val;
    __asm__ volatile("ld %0, 0(%1)" : "=r"(val) : "r"(bait));

    // Both csrw and OOB load succeeded → real vulnerability.
    print("FAIL [csr_tamper]: CSR tamper + OOB access both succeeded!\n");
    raw_exit(1);
}
