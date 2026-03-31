/*
 * sfi_pingpong.c — 模拟 SFI (Software Fault Isolation) 方案的上下文切换开销
 *
 * SFI（如 NaCl、WASM）的上下文切换与 DASICS 的本质区别：
 *   - 无 ecall/URET 硬件陷入（用普通 call/ret）
 *   - 无 DASICS CSR 边界寄存器写入
 *   - 只需 save/restore callee-saved 寄存器 + 切换 guard register
 *
 * 本测试在两个 SFI 风格的 context 之间做乒乓切换，
 * 每个 context 有独立的栈和一个 guard register（模拟沙箱基址寄存器）。
 *
 * 这是一个 native 程序（不在 LPS 沙箱内运行），直接编译运行：
 *   riscv64-linux-gcc -static -O2 -o bin/sfi_pingpong sfi_pingpong.c
 *   ./bin/sfi_pingpong
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cycle.h"

#define ITERATIONS 100000
#define STACK_SIZE (64 * 1024)

/*
 * SFI context: callee-saved registers + guard register
 * 模拟 SFI 运行时在切换沙箱时需要保存/恢复的状态
 */
typedef struct {
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
    uint64_t guard;     /* SFI sandbox base register (e.g., NaCl 用一个寄存器存沙箱基址) */
} SFIContext;

/*
 * sfi_switch(SFIContext *old, SFIContext *new)
 *
 * 模拟 SFI 上下文切换：纯 callee-saved 寄存器 save/restore + guard register
 * 无 ecall、无 URET、无 CSR 操作
 */
__asm__ (
    ".p2align 4\n"
    ".global sfi_switch\n"
    "sfi_switch:\n"
    "    # save old context (a0 = old)\n"
    "    sd ra,   0(a0)\n"
    "    sd sp,   8(a0)\n"
    "    sd s0,  16(a0)\n"
    "    sd s1,  24(a0)\n"
    "    sd s2,  32(a0)\n"
    "    sd s3,  40(a0)\n"
    "    sd s4,  48(a0)\n"
    "    sd s5,  56(a0)\n"
    "    sd s6,  64(a0)\n"
    "    sd s7,  72(a0)\n"
    "    sd s8,  80(a0)\n"
    "    sd s9,  88(a0)\n"
    "    sd s10, 96(a0)\n"
    "    sd s11, 104(a0)\n"
    "    sd tp,  112(a0)\n"     /* 用 tp 模拟 guard register */
    "    # load new context (a1 = new)\n"
    "    ld ra,   0(a1)\n"
    "    ld sp,   8(a1)\n"
    "    ld s0,  16(a1)\n"
    "    ld s1,  24(a1)\n"
    "    ld s2,  32(a1)\n"
    "    ld s3,  40(a1)\n"
    "    ld s4,  48(a1)\n"
    "    ld s5,  56(a1)\n"
    "    ld s6,  64(a1)\n"
    "    ld s7,  72(a1)\n"
    "    ld s8,  80(a1)\n"
    "    ld s9,  88(a1)\n"
    "    ld s10, 96(a1)\n"
    "    ld s11, 104(a1)\n"
    "    ld tp,  112(a1)\n"     /* restore guard register */
    "    ret\n"
);

extern void sfi_switch(SFIContext *old, SFIContext *new_ctx);

/* 共享状态 */
static SFIContext ctx_a, ctx_b;
static volatile int remaining;

/*
 * pong 侧的入口函数：被切换到后，立即切换回 ping
 */
static void pong_entry(void)
{
    while (remaining > 0) {
        sfi_switch(&ctx_b, &ctx_a);  /* B→A */
    }
    /* 最后一次切回 A 让它结束 */
    sfi_switch(&ctx_b, &ctx_a);
}

int main(void)
{
    /* 分配 pong 的栈 */
    void *stack_b = malloc(STACK_SIZE);
    if (!stack_b) {
        perror("malloc");
        return 1;
    }
    void *stack_b_top = stack_b + STACK_SIZE;

    /* 初始化 ctx_b: ra=pong_entry, sp=栈顶 */
    memset(&ctx_b, 0, sizeof(ctx_b));
    ctx_b.ra = (uint64_t)pong_entry;
    ctx_b.sp = (uint64_t)stack_b_top;
    ctx_b.guard = 0xBBBB000000000000ULL;  /* 模拟 sandbox B 的基址 */

    /* ctx_a 不需要预初始化——sfi_switch 会保存当前状态 */
    memset(&ctx_a, 0, sizeof(ctx_a));
    ctx_a.guard = 0xAAAA000000000000ULL;  /* 模拟 sandbox A 的基址 */

    remaining = ITERATIONS;

    printf("[SFI-PINGPONG] Starting: %d round trips\n", ITERATIONS);

    uint64_t start = get_cycle_count();

    /* ping 侧：每次切到 B，B 立即切回 */
    for (int i = 0; i < ITERATIONS; i++) {
        remaining = ITERATIONS - i;
        sfi_switch(&ctx_a, &ctx_b);  /* A→B, B 会立即切回 A */
    }

    uint64_t end = get_cycle_count();

    uint64_t total = end - start;
    /* 每次循环包含 2 次 sfi_switch (A→B + B→A) */
    uint64_t per_switch = total / (ITERATIONS * 2);
    double total_ns = cycle_to_time_ns(total, FPGA_HZ);
    double per_switch_ns = total_ns / (ITERATIONS * 2.0);

    printf("[SFI-PINGPONG] Results:\n");
    printf("  Total round trips:    %d\n", ITERATIONS);
    printf("  Total switches:       %d\n", ITERATIONS * 2);
    printf("  Total cycles:         %lu\n", total);
    printf("  Cycles per switch:    %lu\n", per_switch);
    printf("  Time per switch:      %.2f ns (%.2f us)\n",
           per_switch_ns, per_switch_ns / 1000.0);
    printf("\n");
    printf("  This measures: pure callee-saved register save/restore + guard register\n");
    printf("  No ecall/URET, no DASICS CSR, no hardware trap.\n");
    printf("  This simulates SFI (NaCl/WASM) context switch overhead.\n");

    free(stack_b);
    return 0;
}
