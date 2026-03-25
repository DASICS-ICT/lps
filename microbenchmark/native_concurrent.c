// microbenchmark/native_concurrent.c
// Native 对照：fork+exec N 个子进程，每个显式加载 workload ELF 后运行。
// 与 LPS 方案对比：OS 为每个子进程创建独立地址空间并执行 ELF 加载流程，
// LPS 则在同一地址空间用 DASICS 隔离并自行完成 ELF 加载。
//
// 用法：native_concurrent <N> <workload-elf> [fib-count]
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

static inline uint64_t rdcycle(void) {
    uint64_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
}

#define FPGA_HZ 50000000ULL
static double cy2ms(uint64_t cy) { return (double)cy * 1e3 / FPGA_HZ; }

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <N> <workload-elf> [fib-count]\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    char *elf = argv[2];
    char *fib_str = (argc >= 4) ? argv[3] : NULL;
    char *child_argv[] = { elf, fib_str, NULL };

    // 阶段一：fork N 子进程，每个 exec 显式加载 workload ELF
    uint64_t t_fork_start = rdcycle();
    for (int i = 0; i < n; i++) {
        pid_t p = fork();
        if (p < 0) { perror("fork"); exit(1); }
        if (p == 0) {
            execv(elf, child_argv);
            perror("execv");
            _exit(1);
        }
    }
    for (int i = 0; i < n; i++) wait(NULL);
    uint64_t t_wait_end = rdcycle();
    uint64_t cy_total = t_wait_end - t_fork_start;

    fprintf(stderr, "[native] N=%d\n", n);
    fprintf(stderr, "  fork + run: %llu cycles  %.1f ms\n",
            (unsigned long long)cy_total, cy2ms(cy_total));
    return 0;
}
