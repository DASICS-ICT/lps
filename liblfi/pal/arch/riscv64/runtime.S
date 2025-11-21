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
    sd t0,  OFFSET_UEPC(s0)
    // save ucasue
    csrr t0, CSR_UCAUSE
    sd t0,  OFFSET_UCAUSE(s0)
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
    ld t0, OFFSET_UEPC(a0)
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

// LightSerial prologue
.macro LS_PROLOGUE
    csrr t6, 0x880
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


// kswitch(void* p, KContext *old, KContext *new)
.p2align 4
.global kswitch
kswitch:
    // save old context
    sd ra,  CONTEXT_RA(a1)
    sd sp,  CONTEXT_SP(a1)
    sd s0,  CONTEXT_S0(a1)
    sd s1,  CONTEXT_S1(a1)
    sd s2,  CONTEXT_S2(a1)
    sd s3,  CONTEXT_S3(a1)
    sd s4,  CONTEXT_S4(a1)
    sd s5,  CONTEXT_S5(a1)
    sd s6,  CONTEXT_S6(a1)
    sd s7,  CONTEXT_S7(a1)
    sd s8,  CONTEXT_S8(a1)
    sd s9,  CONTEXT_S9(a1)
    sd s10, CONTEXT_S10(a1)
    sd s11, CONTEXT_S11(a1)
    // load new context
    ld ra,  CONTEXT_RA(a2)
    ld sp,  CONTEXT_SP(a2)
    ld s0,  CONTEXT_S0(a2)
    ld s1,  CONTEXT_S1(a2)
    ld s2,  CONTEXT_S2(a2)
    ld s3,  CONTEXT_S3(a2)
    ld s4,  CONTEXT_S4(a2)
    ld s5,  CONTEXT_S5(a2)
    ld s6,  CONTEXT_S6(a2)
    ld s7,  CONTEXT_S7(a2)
    ld s8,  CONTEXT_S8(a2)
    ld s9,  CONTEXT_S9(a2)
    ld s10, CONTEXT_S10(a2)
    ld s11, CONTEXT_S11(a2)
    ret

// -------------------------------------------------------------------------
// lps_ctx_entry(LFIContext* ctx)
// a0: LFIContext* ctx
// -------------------------------------------------------------------------
.p2align 4
.global lps_ctx_entry
lps_ctx_entry:
    // save k_ctx's sp, runtime's gp & tp
    sd sp, OFFSET_KSP(a0)
    sd gp, OFFSET_KGP(a0)
    sd tp, OFFSET_KTP(a0)
    RESTORE_ALL
    ebreak

// void init_dasics()
.p2align 4
.global lps_init_dasics
lps_init_dasics:
    la t0, dasics_ufault_entry
    csrw CSR_UTVEC, t0
    // set ustatus.UPIE to set UIE then by uret
    csrwi CSR_USTATUS, 0x10
    li t0, 0x111
    csrw CSR_UIE, t0
    ret

// void lps_load_dasics(struct LFIContext* ctx)
.p2align 4
.global lps_load_dasics
lps_load_dasics:
    LS_PROLOGUE // csrr t6, 0x880
    // libbound
    ld t0, OFFSET_MEMBOUND0LO(a0)
    ld t1, OFFSET_MEMBOUND0HI(a0)
    csrw CSR_DLBOUND0LO, t0
    csrw CSR_DLBOUND0HI, t1
    ld t0, OFFSET_MEMBOUND1LO(a0)
    ld t1, OFFSET_MEMBOUND1HI(a0)
    csrw CSR_DLBOUND1LO, t0
    csrw CSR_DLBOUND1HI, t1
    ld t0, OFFSET_MEMBOUND2LO(a0)
    ld t1, OFFSET_MEMBOUND2HI(a0)
    csrw CSR_DLBOUND2LO, t0
    csrw CSR_DLBOUND2HI, t1
    ld t0, OFFSET_MEMBOUND3LO(a0)
    ld t1, OFFSET_MEMBOUND3HI(a0)
    csrw CSR_DLBOUND3LO, t0
    csrw CSR_DLBOUND3HI, t1
    ld t0, OFFSET_MEMBOUND4LO(a0)
    ld t1, OFFSET_MEMBOUND4HI(a0)
    csrw CSR_DLBOUND4LO, t0
    csrw CSR_DLBOUND4HI, t1
    ld t0, OFFSET_MEMBOUND5LO(a0)
    ld t1, OFFSET_MEMBOUND5HI(a0)
    csrw CSR_DLBOUND5LO, t0
    csrw CSR_DLBOUND5HI, t1
    ld t0, OFFSET_MEMBOUND6LO(a0)
    ld t1, OFFSET_MEMBOUND6HI(a0)
    csrw CSR_DLBOUND6LO, t0
    csrw CSR_DLBOUND6HI, t1
    ld t0, OFFSET_MEMBOUND7LO(a0)
    ld t1, OFFSET_MEMBOUND7HI(a0)
    csrw CSR_DLBOUND7LO, t0
    csrw CSR_DLBOUND7HI, t1
    ld t0, OFFSET_MEMBOUND8LO(a0)
    ld t1, OFFSET_MEMBOUND8HI(a0)
    csrw CSR_DLBOUND8LO, t0
    csrw CSR_DLBOUND8HI, t1
    ld t0, OFFSET_MEMBOUND9LO(a0)
    ld t1, OFFSET_MEMBOUND9HI(a0)
    csrw CSR_DLBOUND9LO, t0
    csrw CSR_DLBOUND9HI, t1
    ld t0, OFFSET_MEMBOUND10LO(a0)
    ld t1, OFFSET_MEMBOUND10HI(a0)
    csrw CSR_DLBOUND10LO, t0
    csrw CSR_DLBOUND10HI, t1
    ld t0, OFFSET_MEMBOUND11LO(a0)
    ld t1, OFFSET_MEMBOUND11HI(a0)
    csrw CSR_DLBOUND11LO, t0
    csrw CSR_DLBOUND11HI, t1
    ld t0, OFFSET_MEMBOUND12LO(a0)
    ld t1, OFFSET_MEMBOUND12HI(a0)
    csrw CSR_DLBOUND12LO, t0
    csrw CSR_DLBOUND12HI, t1
    ld t0, OFFSET_MEMBOUND13LO(a0)
    ld t1, OFFSET_MEMBOUND13HI(a0)
    csrw CSR_DLBOUND13LO, t0
    csrw CSR_DLBOUND13HI, t1
    ld t0, OFFSET_MEMBOUND14LO(a0)
    ld t1, OFFSET_MEMBOUND14HI(a0)
    csrw CSR_DLBOUND14LO, t0
    csrw CSR_DLBOUND14HI, t1
    ld t0, OFFSET_MEMBOUND15LO(a0)
    ld t1, OFFSET_MEMBOUND15HI(a0)
    csrw CSR_DLBOUND15LO, t0
    csrw CSR_DLBOUND15HI, t1

    // jmpbound
    ld t0, OFFSET_JMPBOUND0LO(a0)
    ld t1, OFFSET_JMPBOUND0HI(a0)
    csrw CSR_DJBOUND0LO, t0
    csrw CSR_DJBOUND0HI, t1
    ld t0, OFFSET_JMPBOUND1LO(a0)
    ld t1, OFFSET_JMPBOUND1HI(a0)
    csrw CSR_DJBOUND1LO, t0
    csrw CSR_DJBOUND1HI, t1
    ld t0, OFFSET_JMPBOUND2LO(a0)
    ld t1, OFFSET_JMPBOUND2HI(a0)
    csrw CSR_DJBOUND2LO, t0
    csrw CSR_DJBOUND2HI, t1
    ld t0, OFFSET_JMPBOUND3LO(a0)
    ld t1, OFFSET_JMPBOUND3HI(a0)
    csrw CSR_DJBOUND3LO, t0
    csrw CSR_DJBOUND3HI, t1
    
    // cfg
    ld t0, OFFSET_MEMCFG(a0)
    ld t1, OFFSET_JMPCFG(a0)
    csrw CSR_DLCFG, t0
    csrw CSR_DJCFG, t1
    ret

// void lps_load_float(struct LFIContext* ctx)
.p2align 4
.global lps_load_float
lps_load_float:
fld f0, OFFSET_F+8*0(a0)
    fld f1, OFFSET_F+8*1(a0)
    fld f2, OFFSET_F+8*2(a0)
    fld f3, OFFSET_F+8*3(a0)
    fld f4, OFFSET_F+8*4(a0)
    fld f5, OFFSET_F+8*5(a0)
    fld f6, OFFSET_F+8*6(a0)
    fld f7, OFFSET_F+8*7(a0)
    fld f8, OFFSET_F+8*8(a0)
    fld f9, OFFSET_F+8*9(a0)
    fld f10, OFFSET_F+8*10(a0)
    fld f11, OFFSET_F+8*11(a0)
    fld f12, OFFSET_F+8*12(a0)
    fld f13, OFFSET_F+8*13(a0)
    fld f14, OFFSET_F+8*14(a0)
    fld f15, OFFSET_F+8*15(a0)
    fld f16, OFFSET_F+8*16(a0)
    fld f17, OFFSET_F+8*17(a0)
    fld f18, OFFSET_F+8*18(a0)
    fld f19, OFFSET_F+8*19(a0)
    fld f20, OFFSET_F+8*20(a0)
    fld f21, OFFSET_F+8*21(a0)
    fld f22, OFFSET_F+8*22(a0)
    fld f23, OFFSET_F+8*23(a0)
    fld f24, OFFSET_F+8*24(a0)
    fld f25, OFFSET_F+8*25(a0)
    fld f26, OFFSET_F+8*26(a0)
    fld f27, OFFSET_F+8*27(a0)
    fld f28, OFFSET_F+8*28(a0)
    fld f29, OFFSET_F+8*29(a0)
    fld f30, OFFSET_F+8*30(a0)
    fld f31, OFFSET_F+8*31(a0)
    ret


// void lps_save_float(struct LFIContext* ctx)
.p2align 4
.global lps_save_float
lps_save_float:
    fsd f0, OFFSET_F+8*0(a0)
    fsd f1, OFFSET_F+8*1(a0)
    fsd f2, OFFSET_F+8*2(a0)
    fsd f3, OFFSET_F+8*3(a0)
    fsd f4, OFFSET_F+8*4(a0)
    fsd f5, OFFSET_F+8*5(a0)
    fsd f6, OFFSET_F+8*6(a0)
    fsd f7, OFFSET_F+8*7(a0)
    fsd f8, OFFSET_F+8*8(a0)
    fsd f9, OFFSET_F+8*9(a0)
    fsd f10, OFFSET_F+8*10(a0)
    fsd f11, OFFSET_F+8*11(a0)
    fsd f12, OFFSET_F+8*12(a0)
    fsd f13, OFFSET_F+8*13(a0)
    fsd f14, OFFSET_F+8*14(a0)
    fsd f15, OFFSET_F+8*15(a0)
    fsd f16, OFFSET_F+8*16(a0)
    fsd f17, OFFSET_F+8*17(a0)
    fsd f18, OFFSET_F+8*18(a0)
    fsd f19, OFFSET_F+8*19(a0)
    fsd f20, OFFSET_F+8*20(a0)
    fsd f21, OFFSET_F+8*21(a0)
    fsd f22, OFFSET_F+8*22(a0)
    fsd f23, OFFSET_F+8*23(a0)
    fsd f24, OFFSET_F+8*24(a0)
    fsd f25, OFFSET_F+8*25(a0)
    fsd f26, OFFSET_F+8*26(a0)
    fsd f27, OFFSET_F+8*27(a0)
    fsd f28, OFFSET_F+8*28(a0)
    fsd f29, OFFSET_F+8*29(a0)
    fsd f30, OFFSET_F+8*30(a0)
    fsd f31, OFFSET_F+8*31(a0)
    ret

#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
