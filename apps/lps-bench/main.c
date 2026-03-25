#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "lps_linux.h"

static inline uint64_t rdcycle(void) {
    uint64_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
}

#define FPGA_HZ 50000000ULL
static double cy2ms(uint64_t cy) { return (double)cy * 1e3 / FPGA_HZ; }

static size_t mb(size_t x) { return x * 1024 * 1024; }

static struct LPSThread *
gen_thread(struct LPSLinuxEngine *eng, int fd, const char *elf, const char *fib_str)
{
    struct LPSProc *proc = lps_proc_new(eng);
    assert(proc);
    assert(lps_proc_load_fd(proc, fd, elf));
    const char *env = NULL;
    const char *args[3] = {elf, fib_str, NULL};
    int nargs = fib_str ? 2 : 1;
    return lps_thread_new(proc, nargs, args, &env);
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <N> <ncore> <elf> [fib-count]\n", argv[0]);
        return 1;
    }
    int n     = atoi(argv[1]);
    int ncore = atoi(argv[2]);
    const char *elf     = argv[3];
    const char *fib_str = (argc >= 5) ? argv[4] : NULL;

    struct LPSEngine *engine = lps_new((struct LPSOptions){
        .boxsize = mb(64), .verbose = false,
    }, n + 4);
    assert(engine);

    struct LPSLinuxEngine *x_engine = lps_linux_new(engine,
        (struct LPSLinuxOpts){ .brksize = mb(4), .stacksize = mb(1), .verbose = false});
    assert(x_engine);

    mcsched_init(ncore);

    // 阶段一：创建所有沙箱
    uint64_t t_create_start = rdcycle();
    FILE *f = fopen(elf, "r");
    for (int i = 0; i < n; i++)
        mcsched_add(gen_thread(x_engine, fileno(f), elf, fib_str));
    fclose(f);
    uint64_t t_create_end = rdcycle();

    // 阶段二：执行所有沙箱直到完成
    uint64_t t_run_start = rdcycle();
    mcshed_start();
    mcsched_join();
    uint64_t t_run_end = rdcycle();

    uint64_t cy_create = t_create_end - t_create_start;
    uint64_t cy_run    = t_run_end    - t_run_start;
    uint64_t cy_total  = t_run_end    - t_create_start;

    fprintf(stderr, "[lps] N=%d ncore=%d\n", n, ncore);
    fprintf(stderr, "  create: %llu cycles  %.1f ms\n",
            (unsigned long long)cy_create, cy2ms(cy_create));
    fprintf(stderr, "  run:    %llu cycles  %.1f ms\n",
            (unsigned long long)cy_run,    cy2ms(cy_run));
    fprintf(stderr, "  total:  %llu cycles  %.1f ms\n",
            (unsigned long long)cy_total,  cy2ms(cy_total));
    return 0;
}
