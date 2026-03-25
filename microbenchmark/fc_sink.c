// microbenchmark/fc_sink.c
// 函数链测试 - 数据消费者：读 fd=3，校验并统计，rdcycle 测管道吞吐时间
#include <unistd.h>
#include <stdio.h>
#include "cycle.h"

#define CHUNK 4096

int main(void) {
    char buf[CHUNK];
    ssize_t total = 0;
    long chksum = 0;
    ssize_t n;

    uint64_t t0 = get_cycle_count();
    while ((n = read(3, buf, CHUNK)) > 0) {
        for (ssize_t i = 0; i < n; i++) chksum += (unsigned char)buf[i];
        total += n;
    }
    uint64_t t1 = get_cycle_count();

    double ms = (double)cycle_to_time_ns(t1 - t0, FPGA_HZ) / 1e6;
    fprintf(stderr, "[sink] received=%zd bytes, checksum=%ld, time=%.1f ms, bw=%.1f KB/s\n",
            total, chksum, ms, ms > 0 ? (double)total / ms : 0.0);
    return 0;
}
