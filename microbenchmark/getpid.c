#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include "cycle.h"

#define ITERATIONS 10000

int main(void) {
    uint64_t start, end;
    pid_t pid;
    
    printf("迭代次数: %d\n", ITERATIONS);
    start = get_cycle_count();
    for (int i = 0; i < ITERATIONS; i++) {
        pid = getpid();
        // 防止编译器优化
        asm volatile("" : : "r"(pid) : "memory");
    }
    end = get_cycle_count();
    
    uint64_t all_cycles = end - start;

    uint64_t avg_cycles = all_cycles / ITERATIONS;
    double all_time_ns = cycle_to_time_ns(all_cycles, FPGA_HZ);
    double avg_time_ns = all_time_ns / (ITERATIONS);
    
    printf("Iterations: %d\n", ITERATIONS);
    printf("Total cycles: %llu\n", all_cycles);
    printf("Total time: %.2f ms\n", all_time_ns / 1000000.0);
    printf("Cycles per syscall: %llu\n", avg_cycles);
    printf("Time per syscall: %.2f μs\n", avg_time_ns / 1000.0);
    
    return 0;
}