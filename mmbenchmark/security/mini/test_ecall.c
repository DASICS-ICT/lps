// test_ecall.c - DASICS ecall fault security test
//
// Proves that ecall from sandbox code is intercepted by DASICS (DFR_EF),
// NOT passed directly to the kernel.
//
// Method: issue an ecall with a non-existent syscall number (9999).
//   - If DASICS intercepts: the LPS handler recognizes the marker,
//     prints PASS, and returns 0 to the sandbox.
//   - If NOT intercepted: the kernel receives sysno=9999, returns -ENOSYS,
//     and no PASS is printed.
//
// The PASS verdict comes from the runner (trusted host code), not from
// the sandbox itself, so it cannot be faked.

#include <stdint.h>

// Marker syscall number - does not exist in Linux.
// The lps-security handler recognizes it and returns 0.
// The kernel would return -ENOSYS.
#define ECALL_TEST_MARKER 9999

static long raw_ecall_marker(void) {
    register long a0 __asm__("a0") = 0;
    register long a7 __asm__("a7") = ECALL_TEST_MARKER;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a7)
        : "memory");
    return a0;
}

static void __attribute__((noreturn)) raw_exit(int code) {
    register long a0 __asm__("a0") = code;
    register long a7 __asm__("a7") = 94; // SYS_exit_group
    __asm__ volatile("ecall" : : "r"(a0), "r"(a7));
    __builtin_unreachable();
}

int main(void) {
    long ret = raw_ecall_marker();
    // ret == 0  => DASICS intercepted, handler returned success
    // ret != 0  => kernel handled (returned -ENOSYS), DASICS not active
    raw_exit(ret == 0 ? 0 : 1);
    return 0;
}
