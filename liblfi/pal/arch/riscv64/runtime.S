#include "config.h"
#include "pal/arch/riscv64/regs.h"
#include "pal/arch_asm.h"

#define N_RTCALLS 256
#define CFG_REGS  320

// a0: LFIContext* ctx
.macro ENTER_DASICS
    la t0, dasics_ufault_entry
    csrw CSR_UTVEC, t0
    // libbound
    ld t0, OFFSET_MEMBOUND0LO(a0)
    ld t1, OFFSET_MEMBOUND0HI(a0)
    csrw CSR_DLBOUND0LO, t0
    csrw CSR_DLBOUND0HI, t1
    // jmpbound
    ld t0, OFFSET_JMPBOUND0LO(a0)
    ld t1, OFFSET_JMPBOUND0HI(a0)
    csrw CSR_DJBOUND0LO, t0
    csrw CSR_DJBOUND0HI, t1
    // cfg
    ld t0, OFFSET_MEMCFG(a0)
    csrw CSR_DLCFG, t0
    ld t1, OFFSET_JMPCFG(a0)
    csrw CSR_DJCFG, t1
.endm

// LFIContext* ctx in CSR_USCRATCH
.macro SAVE_ALL
    csrrw s0, CSR_USCRATCH, s0 // CSR_USCRATCH
    sd ra,  OFFSET_RA(s0)
    sd sp,  OFFSET_SP(s0)
    sd gp,  OFFSET_GP(s0)
    sd tp,  OFFSET_TP(s0)
    sd t0,  OFFSET_T0(s0)
    sd t1,  OFFSET_T1(s0)
    sd t2,  OFFSET_T2(s0)
    // sd s0,  OFFSET_S0(s0)
    sd s1,  OFFSET_S1(s0)
    sd a0,  OFFSET_A0(s0)
    sd a1,  OFFSET_A1(s0)
    sd a2,  OFFSET_A2(s0)
    sd a3,  OFFSET_A3(s0)
    sd a4,  OFFSET_A4(s0)
    sd a5,  OFFSET_A5(s0)
    sd a6,  OFFSET_A6(s0)
    sd a7,  OFFSET_A7(s0)
    sd s2,  OFFSET_S2(s0)
    sd s3,  OFFSET_S3(s0)
    sd s4,  OFFSET_S4(s0)
    sd s5,  OFFSET_S5(s0)
    sd s6,  OFFSET_S6(s0)
    sd s7,  OFFSET_S7(s0)
    sd s8,  OFFSET_S8(s0)
    sd s9,  OFFSET_S9(s0)
    sd s10, OFFSET_S10(s0)
    sd s11, OFFSET_S11(s0)
    sd t3,  OFFSET_T3(s0)
    sd t4,  OFFSET_T4(s0)
    sd t5,  OFFSET_T5(s0)
    sd t6,  OFFSET_T6(s0)
    // save s0
    csrr t0, CSR_USCRATCH
    sd t0,  OFFSET_S0(s0)
    // save uepc
    csrr t0, CSR_UEPC
    sd t0,  OFFSET_UPEC(s0)
    // save utval
    csrr t0, CSR_UTVAL
    sd t0,  OFFSET_UTVAL(s0)
    // save dfreason
    csrr t0, CSR_DFREASON
    sd t0,  OFFSET_DFREN(s0)
    // change into runtime's sp, gp, tp
    ld sp, OFFSET_KSP(s0)
    ld gp, OFFSET_KGP(s0)
    ld tp, OFFSET_KTP(s0)
.endm

// restore_all(LFIContext *)
// a0: LFIContext *
.macro RESTORE_ALL
    // save uscratch
    csrw CSR_USCRATCH, a0
    // load uepc
    ld t0, OFFSET_UPEC(a0)
    csrw CSR_UEPC, t0
    ld ra,  OFFSET_RA(a0)
    ld sp,  OFFSET_SP(a0)
    ld gp,  OFFSET_GP(a0)
    ld tp,  OFFSET_TP(a0)
    ld t0,  OFFSET_T0(a0)
    ld t1,  OFFSET_T1(a0)
    ld t2,  OFFSET_T2(a0)
    ld s0,  OFFSET_S0(a0)
    ld s1,  OFFSET_S1(a0)
    // ld a0,  OFFSET_A0(a0)
    ld a1,  OFFSET_A1(a0)
    ld a2,  OFFSET_A2(a0)
    ld a3,  OFFSET_A3(a0)
    ld a4,  OFFSET_A4(a0)
    ld a5,  OFFSET_A5(a0)
    ld a6,  OFFSET_A6(a0)
    ld a7,  OFFSET_A7(a0)
    ld s2,  OFFSET_S2(a0)
    ld s3,  OFFSET_S3(a0)
    ld s4,  OFFSET_S4(a0)
    ld s5,  OFFSET_S5(a0)
    ld s6,  OFFSET_S6(a0)
    ld s7,  OFFSET_S7(a0)
    ld s8,  OFFSET_S8(a0)
    ld s9,  OFFSET_S9(a0)
    ld s10, OFFSET_S10(a0)
    ld s11, OFFSET_S11(a0)
    ld t3,  OFFSET_T3(a0)
    ld t4,  OFFSET_T4(a0)
    ld t5,  OFFSET_T5(a0)
    ld t6,  OFFSET_T6(a0)
    ld a0,  OFFSET_A0(a0)
    uret
.endm

.p2align 4
.global lfi_ctx_internal
// This function places its return value in x8
lfi_ctx_internal:
	ebreak

// -------------------------------------------------------------------------
// lfi_ctx_entry(LFIContext* ctx)
// a0: LFIContext* ctx
// -------------------------------------------------------------------------
.p2align 4
.global lfi_ctx_entry
lfi_ctx_entry:
    // 12 * 8 = 96 bytes
    addi sp, sp, -96
    // save callee-saved registers to stack
    sd ra, 8*0(sp)
    sd s0, 8*1(sp)
    sd s1, 8*2(sp)
    sd s2, 8*3(sp)
    sd s3, 8*4(sp)
    sd s4, 8*5(sp)
    sd s5, 8*6(sp)
    sd s6, 8*7(sp)
    sd s7, 8*8(sp)
    sd s8, 8*9(sp)
    sd s9, 8*10(sp)
    sd s10, 8*11(sp)
    // save stack to kstackp
    sd sp, OFFSET_KSP(a0)
    sd gp, OFFSET_KGP(a0)
    sd tp, OFFSET_KTP(a0)
    ENTER_DASICS
    RESTORE_ALL
    ebreak

// -------------------------------------------------------------------------
// lfi_asm_ctx_exit(void* kstackp, uint64_t code)
// a0: void* kstackp
// a1: uint64_t code
// -------------------------------------------------------------------------
.p2align 4
.global lfi_asm_ctx_exit
lfi_asm_ctx_exit:
    mv sp, a0
    mv a0, a1
    ld ra, 8*0(sp)
    ld s0, 8*1(sp)
    ld s1, 8*2(sp)
    ld s2, 8*3(sp)
    ld s3, 8*4(sp)
    ld s4, 8*5(sp)
    ld s5, 8*6(sp)
    ld s6, 8*7(sp)
    ld s7, 8*8(sp)
    ld s8, 8*9(sp)
    ld s9, 8*10(sp)
    ld s10, 8*11(sp)
    addi sp, sp, 96
    ret

.p2align 4
.global dasics_ufault_entry
dasics_ufault_entry:
    SAVE_ALL
    mv a0, s0
    jal lfi_syscall_handler
    mv a0, s0
    RESTORE_ALL

#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
