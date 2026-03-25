// microbenchmark/native_startup.c
// 测量 fork()+execv() 启动一个最简程序的完整延迟，用 rdcycle 采样。
// 与 lps-startup 对比：fork() ↔ lps_proc_new+elf_load+thread_new，
//                      fork+exec+wait ↔ lps_thread_run 整体延迟。
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

static inline uint64_t rdcycle(void) {
    uint64_t c; asm volatile("rdcycle %0" : "=r"(c)); return c;
}

#define FPGA_HZ 50000000ULL
static double cy2us(uint64_t cy) { return (double)cy * 1e6 / FPGA_HZ; }

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <exit_imm_binary>\n", argv[0]);
        return 1;
    }

    const int ROUNDS = 20;
    uint64_t sum_fork = 0, sum_total = 0;

    for (int i = 0; i < ROUNDS; i++) {
        uint64_t t0 = rdcycle();
        pid_t p = fork();
        if (p == 0) {
            char *args[] = { argv[1], NULL };
            execv(argv[1], args);
            _exit(1);
        }
        assert(p > 0);
        int status;
        waitpid(p, &status, 0);
        uint64_t t2 = rdcycle();

        sum_total += t2 - t0;
    }

    printf("Native fork+exec latency (avg over %d runs, FPGA@%lluMHz):\n",
           ROUNDS, (unsigned long long)(FPGA_HZ / 1000000));
    printf("  fork+exec+run: %.1f us  (%llu cycles)\n",
           cy2us(sum_total / ROUNDS), (unsigned long long)(sum_total / ROUNDS));
    return 0;
}
