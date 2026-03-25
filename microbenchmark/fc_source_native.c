// microbenchmark/fc_source_native.c
// Native 对照 - 数据产生者，写 stdout
#include <unistd.h>
#include <stdio.h>
#include "cycle.h"

#define CHUNK  4096
#define TOTAL  (512 * 1024)

int main(void) {
    char buf[CHUNK];
    for (int i = 0; i < CHUNK; i++) buf[i] = (char)(i & 0xFF);

    uint64_t t0 = get_cycle_count();
    ssize_t sent = 0;
    while (sent < TOTAL) {
        ssize_t n = write(STDOUT_FILENO, buf, CHUNK);
        if (n <= 0) break;
        sent += n;
    }
    uint64_t t1 = get_cycle_count();

    double ms = (double)cycle_to_time_ns(t1 - t0, FPGA_HZ) / 1e6;
    if (ms > 0)
        fprintf(stderr, "[native-source] sent=%zd bytes, time=%.1f ms, bw=%.1f KB/s\n",
                sent, ms, (double)sent / ms);
    else
        fprintf(stderr, "[native-source] sent=%zd bytes\n", sent);
    return 0;
}
