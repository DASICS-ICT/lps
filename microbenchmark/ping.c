#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define ITERATIONS 100000

// 直接使用约定的文件描述符
#define PIPE_READ_FD  3  // 从pong读取
#define PIPE_WRITE_FD 4  // 向pong写入

// 使用 RISC-V 汇编实现时间读取
static inline uint64_t get_timestamp_ns() {
    uint64_t time;
    asm volatile("rdtime %0" : "=r"(time));
    return time;
}

int main(int argc, char *argv[]) {
    char ping_msg = 'P';
    char pong_msg;
    uint64_t start_time, end_time;
    
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
    start_time = get_timestamp_ns();
    
    for (int i = 0; i < ITERATIONS; i++) {
        write(PIPE_WRITE_FD, &ping_msg, 1);
        read(PIPE_READ_FD, &pong_msg, 1);
    }
    
    end_time = get_timestamp_ns();
    
    uint64_t elapsed_ns = end_time - start_time;
    printf("[PING] Results:\n");
    printf("  Iterations: %d\n", ITERATIONS);
    printf("  Total time: %.3f ms\n", elapsed_ns / 1000000.0);
    printf("  Context switches: %d\n", ITERATIONS * 2);
    printf("  Time per switch: %.1f ns\n", elapsed_ns / (ITERATIONS * 2.0));
    
    return 0;
}
