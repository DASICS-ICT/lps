// test_jump.c - DASICS jump fault security test (function pointer call)
//
// Attempts to call (jalr) an address outside the sandbox's DASICS jump
// bounds. Simulates a code-injection or control-flow hijack attack where
// the attacker redirects a function pointer to arbitrary code.
//
// The target address (passed via argv[1]) points to a host-mapped page,
// ensuring a DASICS jump fault (DFR_JF) instead of a page fault.
//
// Expected: DASICS intercepts the jalr instruction and raises a jump fault.

#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    uintptr_t target = (uintptr_t)strtoul(argv[1], NULL, 16);

    // Cast target to a function pointer and call it.
    // The compiler generates: jalr ra, <target_reg>, 0
    // DASICS hardware checks if the target is within jmpbound and raises
    // a jump fault (DFR_JF = 4) because the target is outside the box.
    void (*fn)(void) = (void (*)(void))target;
    fn();

    // Should never reach here - DASICS fault occurs at the jalr above
    return 1;
}
