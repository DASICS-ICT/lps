#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "cycle.h"

#define ITERATIONS 10000

// 直接使用约定的文件描述符
#define PIPE_READ_FD  3  // 从pong读取
#define PIPE_WRITE_FD 4  // 向pong写入

int main(int argc, char *argv[]) {
    char ping_msg = 'P';
    char pong_msg;
    uint64_t start_cycle, end_cycle;
    
    printf("[PING] Starting pingpong test with fd: read=%d, write=%d\n", 
           PIPE_READ_FD, PIPE_WRITE_FD);
    
    // 测试管道是否工作
    if (write(PIPE_WRITE_FD, &ping_msg, 1) != 1) {
        printf("[PING] Failed to write to pipe\n");
        return 1;
    }
    
    if (read(PIPE_READ_FD, &pong_msg, 1) != 1) {
        printf("[PING] Failed to read from pipe\n");
        return 1;
    }
    
    printf("[PING] Pipe test successful, starting benchmark...\n");
    
    // 开始性能测试
    start_cycle = get_cycle_count();
    
    for (int i = 0; i < ITERATIONS; i++) {
        write(PIPE_WRITE_FD, &ping_msg, 1);
        read(PIPE_READ_FD, &pong_msg, 1);
    }
    
    end_cycle = get_cycle_count();
    
    uint64_t all_cycles = end_cycle - start_cycle;
    uint64_t switch_cycles = all_cycles / (ITERATIONS * 2);
    double all_time_ns = cycle_to_time_ns(all_cycles, FPGA_HZ);
    double switch_time_ns = all_time_ns / (ITERATIONS * 2.0);

    printf("[PING] Results:\n");
    printf("  Iterations: %d\n", ITERATIONS);
    printf("  Total cycles: %llu\n", all_cycles);
    printf("  Total time: %.2f ms\n", all_time_ns / 1000000.0);
    printf("  Context switches: %d\n", ITERATIONS * 2);
    printf("  Cycles per switch: %llu\n", switch_cycles);
    printf("  Time per switch: %.2f μs\n", switch_time_ns / 1000.0);
    
    return 0;
}
