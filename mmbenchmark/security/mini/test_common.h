// test_common.h - Shared helpers for LPS DASICS security tests
//
// Provides raw ecall wrappers that bypass libc, so the test can
// exercise the DASICS ecall interception path directly.

#pragma once
#include <stdint.h>
#include <stddef.h>

// --------------- magic values for handler protocol ---------------
// Set a6 to these BEFORE a deliberate fault so the handler knows
// to skip the instruction and continue instead of terminating.
#define SKIP_MAGIC 0x5B1D5B1D5B1D5B1DULL
#define CSR_MAGIC  0xC5B0C5B0C5B0C5B0ULL

// --------------- raw ecall helpers ---------------

static inline long raw_write(int fd, const char *buf, long count) {
    register long a0 __asm__("a0") = fd;
    register long a1 __asm__("a1") = (long)buf;
    register long a2 __asm__("a2") = count;
    register long a7 __asm__("a7") = 64; // SYS_write
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a7)
        : "memory");
    return a0;
}

static inline void __attribute__((noreturn)) raw_exit(int code) {
    register long a0 __asm__("a0") = code;
    register long a7 __asm__("a7") = 94; // SYS_exit_group
    __asm__ volatile("ecall" : : "r"(a0), "r"(a7));
    __builtin_unreachable();
}

// SYS_mmap = 222, flags: MAP_ANONYMOUS=32, MAP_PRIVATE=2, MAP_FIXED=16
static inline long raw_mmap(uintptr_t addr, size_t len, int prot,
                            int flags, int fd, long off) {
    register long a0 __asm__("a0") = (long)addr;
    register long a1 __asm__("a1") = (long)len;
    register long a2 __asm__("a2") = prot;
    register long a3 __asm__("a3") = flags;
    register long a4 __asm__("a4") = fd;
    register long a5 __asm__("a5") = off;
    register long a7 __asm__("a7") = 222; // SYS_mmap
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7)
        : "memory");
    return a0;
}

static inline size_t my_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static inline void print(const char *s) {
    raw_write(1, s, my_strlen(s));
}
