#include "lps_linux.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

static size_t mb(size_t x) { return x * 1024 * 1024; }

static inline uint64_t rdcycle(void) {
    uint64_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
}

#define FPGA_HZ 50000000ULL
static double cy2ms(uint64_t cy) { return (double)cy * 1e3 / FPGA_HZ; }

static struct LPSProc *
make_proc(struct LPSLinuxEngine *eng, const char *elf)
{
    struct LPSProc *p = lps_proc_new(eng);
    assert(p && lps_proc_load_file(p, elf));
    return p;
}

static struct LPSThread *
make_thread(struct LPSProc *p, const char *elf)
{
    const char *env = NULL;
    return lps_thread_new(p, 1, (const char *[]){elf, NULL}, &env);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <source.elf> <transform.elf> <sink.elf>\n", argv[0]);
        return 1;
    }

    struct LPSEngine *engine = lps_new(
        (struct LPSOptions){ .boxsize = mb(64), .verbose = false }, 6);
    assert(engine);

    struct LPSLinuxEngine *x_engine = lps_linux_new(engine,
        (struct LPSLinuxOpts){ .brksize = mb(4), .stacksize = mb(1) });
    assert(x_engine);

    // 三个独立沙箱
    struct LPSProc *src_p = make_proc(x_engine, argv[1]);
    struct LPSProc *xfm_p = make_proc(x_engine, argv[2]);
    struct LPSProc *snk_p = make_proc(x_engine, argv[3]);

    // 建立管道连接：
    //   src.fd4(write) ──▶ xfm.fd3(read)
    //   xfm.fd4(write) ──▶ snk.fd3(read)
    assert(lps_pipe_new(src_p, 4, xfm_p, 3));
    assert(lps_pipe_new(xfm_p, 4, snk_p, 3));

    // 添加到 RR 调度器
    rrschedadd(make_thread(src_p, argv[1]));
    rrschedadd(make_thread(xfm_p, argv[2]));
    rrschedadd(make_thread(snk_p, argv[3]));

    uint64_t t0 = rdcycle();
    rrschedstart(false);
    uint64_t t1 = rdcycle();

    fprintf(stderr, "[lps-chain] total: %llu cycles  %.1f ms\n",
            (unsigned long long)(t1 - t0), cy2ms(t1 - t0));
    return 0;
}
