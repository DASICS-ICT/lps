/*
 * ecall_loop.c — 测量纯 ecall → SAVE_ALL → uepc+=4 → RESTORE_ALL → URET 的开销
 *
 * 在 LPS 沙箱内运行，紧密循环执行 ecall（触发 DASICS EF），
 * syshandle 只做 uepc+=4 并返回，不切换上下文。
 * 这测量的是：硬件陷入延迟 + SAVE_ALL + RESTORE_ALL + URET硬件返回
 * 不包含 DASICS CSR 加载（因为没有上下文切换）。
 *
 * 使用方法：
 *   lps-go bin/ecall_loop -dasics
 *
 * 使用 sysno=500 (runtime_call) + type=一个 nop type 来实现最短路径，
 * 或者直接用一个无害的 syscall（如 getpid）。
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "cycle.h"

#define ITERATIONS 100000

int main(void)
{
    uint64_t start, end;

    /* warm up */
    for (int i = 0; i < 100; i++)
        syscall(SYS_getpid);

    start = get_cycle_count();
    for (int i = 0; i < ITERATIONS; i++) {
        /* getpid: syshandle 直接返回 0，不做 IO/切换 */
        syscall(SYS_getpid);
    }
    end = get_cycle_count();

    uint64_t total = end - start;
    uint64_t per_ecall = total / ITERATIONS;
    double total_ns = cycle_to_time_ns(total, FPGA_HZ);
    double per_ecall_ns = total_ns / ITERATIONS;

    printf("[ECALL-LOOP] Results:\n");
    printf("  Iterations:       %d\n", ITERATIONS);
    printf("  Total cycles:     %lu\n", total);
    printf("  Cycles per ecall: %lu\n", per_ecall);
    printf("  Time per ecall:   %.2f ns (%.2f us)\n",
           per_ecall_ns, per_ecall_ns / 1000.0);
    printf("\n");
    printf("  This measures: ecall trap + SAVE_ALL + syshandle(getpid) + RESTORE_ALL + URET\n");
    printf("  No context switch, no DASICS CSR reload.\n");

    return 0;
}
