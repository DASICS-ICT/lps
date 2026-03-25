#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LPSRegs {
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
    uint64_t ucause;
    uint64_t utval;
    uint64_t dfreason;

    // DASICS bound registers
    uint64_t memcfg;
    uint64_t membound[16][2];
    uint64_t jmpcfg;
    uint64_t jmpbound[4][2];
};

struct KRegs {
    uint64_t ra;
    uint64_t sp;

    uint64_t s0;
    uint64_t s1;
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

    uint64_t sp_base;
};

#define LIBCFG_W (1 << 0)
#define LIBCFG_R (1 << 1)
#define LIBCFG_V (1 << 3)
#define JMPCFG_V (1 << 0)


#ifdef __cplusplus
}
#endif
