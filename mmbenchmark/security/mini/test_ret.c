// test_ret.c - DASICS jump fault security test (return address manipulation)
//
// Overwrites the return address (ra) to point outside the sandbox's DASICS
// jump bounds, then executes a 'ret' instruction. Simulates a ROP (Return-
// Oriented Programming) or buffer-overflow attack where the attacker
// corrupts the return address on the stack.
//
// The target address (passed via argv[1]) points to a host-mapped page,
// ensuring a DASICS jump fault (DFR_JF) instead of a page fault.
//
// Expected: DASICS intercepts the ret (jalr zero, ra, 0) and raises a
//           jump fault.

#include <stdlib.h>
#include <stdint.h>

static void __attribute__((noinline))
trigger_ret_fault(uintptr_t target)
{
    // Overwrite ra with the out-of-bound target, then ret.
    // ret is the pseudo-instruction for: jalr zero, ra, 0
    // DASICS checks ra against jmpbound and raises DFR_JF.
    __asm__ volatile(
        "mv ra, %0\n\t"
        "ret\n\t"
        :
        : "r"(target)
        : "ra"
    );
    __builtin_unreachable();
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    uintptr_t target = (uintptr_t)strtoul(argv[1], NULL, 16);

    trigger_ret_fault(target);

    // Should never reach here - DASICS fault occurs at the ret above
    return 1;
}
