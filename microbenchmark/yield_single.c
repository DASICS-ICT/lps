// simple_ping.c - 简化版ping程序
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "runtime_call.h"

// 使用 RISC-V 汇编实现时间读取
static inline uint64_t get_timestamp_ns() {
    uint64_t time;
    asm volatile("rdtime %0" : "=r"(time));
    return time;
}

// 假设的用户态运行时系统调用
extern void runtime_yield();

#define MAX_ROUNDS 100000

int main() {
  printf("Simple ping starting...\n");
  
  uint64_t start_time = get_timestamp_ns();
  
  for (int i = 0; i < MAX_ROUNDS; i++) {
      // 直接yield，让运行时调度到pong
      runtime_yield();
      
      if (i % 10000 == 0) {
          printf("Ping: Round %d\n", i);
      }
  }
  
  uint64_t end_time = get_timestamp_ns();
  double total_time_ms = (double)(end_time - start_time) / 1000000.0;
  
  printf("Ping completed %d rounds in %.2f ms\n", MAX_ROUNDS, total_time_ms);
  printf("Average time per yield: %.2f ns\n", (double)(end_time - start_time) / MAX_ROUNDS);
  
  return 0;
}

// simple_pong.c - 对应的pong程序
/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int runtime_yield();

#define MAX_ROUNDS 100000

int main() {
  printf("Simple pong starting...\n");
  
  for (int i = 0; i < MAX_ROUNDS; i++) {
      runtime_yield();
      
      if (i % 10000 == 0) {
          printf("Pong: Round %d\n", i);
      }
  }
  
  printf("Pong completed\n");
  return 0;
}
*/