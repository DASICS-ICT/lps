#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__aarch64__) || defined(_M_ARM64)

typedef struct TuxRegs {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
    uint64_t x5;
    uint64_t x6;
    uint64_t x7;
    uint64_t x8;
    uint64_t x9;
    uint64_t x10;
    uint64_t x11;
    uint64_t x12;
    uint64_t x13;
    uint64_t x14;
    uint64_t x15;
    uint64_t x16;
    uint64_t x17;
    uint64_t x18;
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t x29;
    uint64_t x30;
    uint64_t sp;
    uint64_t nzcv;
    uint64_t fpsr;
    uint64_t _pad;
    uint64_t vector[64];
} LFIRegs;

#elif defined(__x86_64__) || defined(_M_X64)

typedef struct TuxRegs {
    uint64_t rsp;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t fs;
    uint64_t gs;
    uint64_t xmm[32];
} LFIRegs;

#elif defined(__riscv) && __riscv_xlen == 64

typedef struct TuxRegs {
    // Host context registers
    uint64_t host_sp;
    uint64_t host_gp;
    uint64_t host_tp;
    uint64_t _pad;

    // RISC-V general purpose registers
    uint64_t zero;
    uint64_t ra;
    uint64_t sp;
    uint64_t gp;
    uint64_t tp;
    uint64_t t0;
    uint64_t t1;
    uint64_t t2;
    uint64_t s0;
	uint64_t s1;
	uint64_t a0;
	uint64_t a1;
	uint64_t a2;
	uint64_t a3;
	uint64_t a4;
	uint64_t a5;
	uint64_t a6;
	uint64_t a7;
	uint64_t s2;
	uint64_t s3;
	uint64_t s4;
	uint64_t s5;
	uint64_t s6;
	uint64_t s7;
	uint64_t s8;
	uint64_t s9;
	uint64_t s10;
	uint64_t s11;
	uint64_t t3;
	uint64_t t4;
	uint64_t t5;
	uint64_t t6;

    // Floating point registers
    uint64_t f[32];

    // DASICS user registers
    uint64_t uepc;    // the entrance of ctx
    uint64_t utval;
    uint64_t dfreason;

    // DASICS bound registers
    uint64_t memcfg;
    uint64_t membound[16][2];
    uint64_t jmpcfg;
    uint64_t jmpbound[4][2];
}LFIRegs;

#endif

#ifdef __cplusplus
}
#endif
