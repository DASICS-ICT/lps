#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "lps_linux.h"

// rdcycle：RISC-V 硬件 cycle 计数器
static inline uint64_t rdcycle(void) {
    uint64_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
}

#define FPGA_HZ 50000000ULL
static double cy2us(uint64_t cy) {
    return (double)cy * 1e6 / FPGA_HZ;
}

static size_t mb(size_t x) { return x * 1024 * 1024; }

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "Usage: %s <elf>\n", argv[0]); return 1; }
    const char *elf = argv[1];

    // 预热引擎（排除 boxmap 初始化的一次性开销）
    struct LPSEngine *engine = lps_new((struct LPSOptions){
        .boxsize = mb(64), .verbose = false,
    }, 32);
    assert(engine);
    struct LPSLinuxEngine *x_engine = lps_linux_new(engine,
        (struct LPSLinuxOpts){ .brksize = mb(8), .stacksize = mb(1) });
    assert(x_engine);

    const int ROUNDS = 20;
    uint64_t sum_proc = 0, sum_load = 0, sum_thread = 0, sum_run = 0;

    for (int i = 0; i < ROUNDS; i++) {
        uint64_t t0 = rdcycle();
        struct LPSProc *proc = lps_proc_new(x_engine);
        uint64_t t1 = rdcycle();
        assert(lps_proc_load_file(proc, elf));
        uint64_t t2 = rdcycle();
        const char *env = NULL;
        struct LPSThread *t = lps_thread_new(proc, 1,
            (const char *[]){elf, NULL}, &env);
        uint64_t t3 = rdcycle();
        lps_thread_run(t);   // 沙箱程序立即 exit(0)
        uint64_t t4 = rdcycle();

        sum_proc   += t1 - t0;
        sum_load   += t2 - t1;
        sum_thread += t3 - t2;
        sum_run    += t4 - t3;
    }

    printf("Startup latency (avg over %d runs, FPGA@%lluMHz):\n",
           ROUNDS, (unsigned long long)(FPGA_HZ / 1000000));
    printf("  proc_new:   %.1f us  (%llu cycles)\n",
           cy2us(sum_proc / ROUNDS),   (unsigned long long)(sum_proc / ROUNDS));
    printf("  elf_load:   %.1f us  (%llu cycles)\n",
           cy2us(sum_load / ROUNDS),   (unsigned long long)(sum_load / ROUNDS));
    printf("  thread_new: %.1f us  (%llu cycles)\n",
           cy2us(sum_thread / ROUNDS), (unsigned long long)(sum_thread / ROUNDS));
    printf("  first_run:  %.1f us  (%llu cycles)\n",
           cy2us(sum_run / ROUNDS),    (unsigned long long)(sum_run / ROUNDS));
    printf("  TOTAL:      %.1f us\n",
        cy2us((sum_proc + sum_load + sum_thread + sum_run) / ROUNDS));
    return 0;
}
