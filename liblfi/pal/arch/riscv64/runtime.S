#include "config.h"
#include "pal/arch/riscv64/regs.h"

#define N_RTCALLS 256
#define PROC_REGS 32
#define JUMP_REGS 256
#define CFG_REGS  320

.macro INIT_DASICS
    la t0, dasics_ufault_entry
    csrw 0x005, t0
    li t0, 0x4000000000
    li t2, 0xb
    csrw 0x890, zero
    csrw 0x891, t0
    csrw 0x880, t2
    fence.i
    li t2, 0x1
    csrw 0x8c0, zero
    csrw 0x8c1, t0
    csrw 0x8c8, t2
    fence.i
.endm

// a1: struct BoundCSR
.macro ENTER_DASICS
    la t0, dasics_ufault_entry
    csrw 0x005, t0
    // libbound
    ld t0, (0*16 + 0*8)(a1)
    ld t1, (0*16 + 1*8)(a1)
    csrw CSR_DLBOUND0LO, t0
    csrw CSR_DLBOUND0HI, t1
    // jmpbound
    ld t0, JUMP_REGS + (0*16 + 0*8)(a1)
    ld t1, JUMP_REGS + (0*16 + 1*8)(a1)
    csrw CSR_DJBOUND0LO, t0
    csrw CSR_DJBOUND0HI, t1
    // cfg
    ld t2, CFG_REGS + (0*8)(a1)
    csrw CSR_DLCFG, t2
    ld t2, CFG_REGS + (1*8)(a1)
    csrw CSR_DJCFG, t2
.endm

.macro SAVE_PROC
    csrrw s0, 0x040, s0 // CSR_USCRATCH
    sd ra, PROC_REGS + 8*1(s0)
    sd sp, PROC_REGS + 8*2(s0)
    sd gp, PROC_REGS + 8*3(s0)
    sd tp, PROC_REGS + 8*4(s0)
    // save caller-saved registers
    sd t0, PROC_REGS + 8*5(s0)
    sd t1, PROC_REGS + 8*6(s0)
    sd t2, PROC_REGS + 8*7(s0)
    sd a0, PROC_REGS + 8*10(s0)
    sd a1, PROC_REGS + 8*11(s0)
    sd a2, PROC_REGS + 8*12(s0)
    sd a3, PROC_REGS + 8*13(s0)
    sd a4, PROC_REGS + 8*14(s0)
    sd a5, PROC_REGS + 8*15(s0)
    sd a6, PROC_REGS + 8*16(s0)
    sd a7, PROC_REGS + 8*17(s0)
    sd t3, PROC_REGS + 8*28(s0)
    sd t4, PROC_REGS + 8*29(s0)
    sd t5, PROC_REGS + 8*30(s0)
    sd t6, PROC_REGS + 8*31(s0)
    // Change into runtime's sp, gp, tp
    ld sp, 0(s0)
    ld gp, 8(s0)
    ld tp, 16(s0)
.endm

.macro RESTORE_PROC
    // uepc += 4
    csrr t0, 0x041
    addi t0, t0, 4
    csrw 0x041, t0
    ld ra, PROC_REGS + 8*1(s0)
    ld sp, PROC_REGS + 8*2(s0)
    ld gp, PROC_REGS + 8*3(s0)
    ld tp, PROC_REGS + 8*4(s0)
    ld t0, PROC_REGS + 8*5(s0)
    ld t1, PROC_REGS + 8*6(s0)
    ld t2, PROC_REGS + 8*7(s0)
    ld a0, PROC_REGS + 8*10(s0)
    ld a1, PROC_REGS + 8*11(s0)
    ld a2, PROC_REGS + 8*12(s0)
    ld a3, PROC_REGS + 8*13(s0)
    ld a4, PROC_REGS + 8*14(s0)
    ld a5, PROC_REGS + 8*15(s0)
    ld a6, PROC_REGS + 8*16(s0)
    ld a7, PROC_REGS + 8*17(s0)
    ld t3, PROC_REGS + 8*28(s0)
    ld t4, PROC_REGS + 8*29(s0)
    ld t5, PROC_REGS + 8*30(s0)
    ld t6, PROC_REGS + 8*31(s0)
    csrrw s0, 0x040, s0 // CSR_USCRATCH
    uret
.endm

.p2align 4
.global lfi_ctx_internal
// This function places its return value in x8
lfi_ctx_internal:
	ebreak

// -------------------------------------------------------------------------
// lfi_ctx_entry(LFIContext* ctx, void* metadata)
// a0: LFIContext* ctx
// a1: void* metadata
// -------------------------------------------------------------------------
.p2align 4
.global lfi_ctx_entry
lfi_ctx_entry:
    csrw 0x040, a0 // CSR_USCRATCH
    ENTER_DASICS
    // 13 * 8 = 104 bytes
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
    sd sp, 8*0(a0)
    sd gp, 8*1(a0)
    sd tp, 8*2(a0)
    j lfi_restore_regs
    ebreak

// -------------------------------------------------------------------------
// lfi_restore_regs
// a0: LFIContext* ctx
// -------------------------------------------------------------------------
.p2align 4
lfi_restore_regs:
    ld ra,  PROC_REGS + 8*1(a0)
    ld sp,  PROC_REGS + 8*2(a0)
    ld gp,  PROC_REGS + 8*3(a0)
    ld tp,  PROC_REGS + 8*4(a0)
    ld t0,  PROC_REGS + 8*5(a0)
    ld t1,  PROC_REGS + 8*6(a0)
    ld t2,  PROC_REGS + 8*7(a0)
    ld s0,  PROC_REGS + 8*8(a0)
    ld s1,  PROC_REGS + 8*9(a0)
    // ld a0,  PROC_REGS + 8*10(a0)
    ld a1,  PROC_REGS + 8*11(a0)
    ld a2,  PROC_REGS + 8*12(a0)
    ld a3,  PROC_REGS + 8*13(a0)
    ld a4,  PROC_REGS + 8*14(a0)
    ld a5,  PROC_REGS + 8*15(a0)
    ld a6,  PROC_REGS + 8*16(a0)
    ld a7,  PROC_REGS + 8*17(a0)
    ld s2,  PROC_REGS + 8*18(a0)
    ld s3,  PROC_REGS + 8*19(a0)
    ld s4,  PROC_REGS + 8*20(a0)
    ld s5,  PROC_REGS + 8*21(a0)
    ld s6,  PROC_REGS + 8*22(a0)
    ld s7,  PROC_REGS + 8*23(a0)
    ld s8,  PROC_REGS + 8*24(a0)
    ld s9,  PROC_REGS + 8*25(a0)
    ld s10, PROC_REGS + 8*26(a0)
    ld s11, PROC_REGS + 8*27(a0)
    ld t3,  PROC_REGS + 8*28(a0)
    ld t4,  PROC_REGS + 8*29(a0)
    ld t5,  PROC_REGS + 8*30(a0)
    ld t6,  PROC_REGS + 8*31(a0)
    ld a0,  PROC_REGS + 8*10(a0)
    ret

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

// Accelerated return for library sandboxes.
.p2align 4
.globl lfi_ret
lfi_ret:
	ebreak

// Puts the sandbox's thread pointer in x0.
.p2align 4
.global lfi_get_tp
lfi_get_tp:
	ebreak

// Sets the sandbox's thread pointer to x0 and clobbers REG_TEMP.
.p2align 4
.global lfi_set_tp
lfi_set_tp:
	ebreak


.p2align 4
.global dasics_ufault_entry
dasics_ufault_entry:
    SAVE_PROC
    csrr t0, 0x8b3 // CSR_DFREASON
    li t1, 1       // DFR_ECALL_DASICS_FAULT
    beq t0, t1, 1f
    ebreak
1:
    mv a0, s0
    jal lfi_syscall_handler
    RESTORE_PROC


.p2align 4
.global lfi_syscall_entry
lfi_syscall_entry:
    ebreak
    
    
.globl lfi_restore_partial_regs
lfi_restore_partial_regs:
	ebreak

#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
