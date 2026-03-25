// microbenchmark/native_multicore.c
// Native 对照：fork N 个子进程，每个显式 execv 加载 workload ELF，
// 按 round-robin 绑定到 ncore 个核上并发运行，父进程 wait 全部完成。
// 与 LPS mcsched 对比：两者均显式加载 ELF 并跨核并发，差异在于
// LPS 共享地址空间 + DASICS 隔离，native 为独立地址空间 + OS 调度。
//
// 用法：native_multicore <N> <ncore> <workload-elf> [fib-count]
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>

static inline uint64_t rdcycle(void) {
    uint64_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
}

#define FPGA_HZ 50000000ULL
static double cy2ms(uint64_t cy) { return (double)cy * 1e3 / FPGA_HZ; }

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s <N> <ncore> <workload-elf> [fib-count]\n", argv[0]);
        return 1;
    }
    int n     = atoi(argv[1]);
    int ncore = atoi(argv[2]);
    char *elf = argv[3];
    char *fib_str = (argc >= 5) ? argv[4] : NULL;
    char *child_argv[] = { elf, fib_str, NULL };

    uint64_t t_fork_start = rdcycle();
    for (int i = 0; i < n; i++) {
        pid_t p = fork();
        if (p < 0) { perror("fork"); exit(1); }
        if (p == 0) {
            // 绑定到 core_id = i % ncore
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(i % ncore, &cpuset);
            sched_setaffinity(0, sizeof(cpuset), &cpuset);
            execv(elf, child_argv);
            perror("execv");
            _exit(1);
        }
    }
    uint64_t t_fork_end = rdcycle();

    for (int i = 0; i < n; i++) wait(NULL);
    uint64_t t_wait_end = rdcycle();

    uint64_t cy_fork  = t_fork_end - t_fork_start;
    uint64_t cy_run   = t_wait_end - t_fork_end;
    uint64_t cy_total = t_wait_end - t_fork_start;

    fprintf(stderr, "[native] N=%d ncore=%d\n", n, ncore);
    fprintf(stderr, "  fork:  %llu cycles  %.1f ms\n",
            (unsigned long long)cy_fork,  cy2ms(cy_fork));
    fprintf(stderr, "  run:   %llu cycles  %.1f ms\n",
            (unsigned long long)cy_run,   cy2ms(cy_run));
    fprintf(stderr, "  total: %llu cycles  %.1f ms\n",
            (unsigned long long)cy_total, cy2ms(cy_total));
    return 0;
}
