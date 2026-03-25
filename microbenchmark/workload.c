// microbenchmark/workload.c
// 固定工作量：斐波那契计算后退出，重点是验证沙箱能正确运行
// 用法：workload [fib-count]  （默认 1000）
#include <stdint.h>
#include <stdlib.h>

static uint64_t fib(uint64_t n) {
    uint64_t a = 0, b = 1;
    for (uint64_t i = 0; i < n; i++) { uint64_t c = a + b; a = b; b = c; }
    return a;
}

int main(int argc, char **argv) {
    uint64_t n = (argc >= 2) ? (uint64_t)atoll(argv[1]) : 1000;
    volatile uint64_t r = fib(n);
    (void)r;
    return 0;
}
