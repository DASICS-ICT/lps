// simple_ping.c - 简化版ping程序
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "runtime_call.h"
#include "cycle.h"

// 假设的用户态运行时系统调用
extern void runtime_yield();

#define MAX_ROUNDS 100000

int main() {
    printf("Yield test starting...\n");
    
    uint64_t start = get_cycle_count();
    
    for (int i = 0; i < MAX_ROUNDS; i++) {
        runtime_yield();
    }
    uint64_t end = get_cycle_count();

    uint64_t all_cycles = end - start;
    uint64_t per_cycles = all_cycles / (MAX_ROUNDS);

    double all_time_ns = cycle_to_time_ns(all_cycles, FPGA_HZ);
    double per_time_ns = all_time_ns / (MAX_ROUNDS * 1.0);

    printf("Iterations: %d\n", MAX_ROUNDS);
    printf("Total cycles: %llu\n", all_cycles);
    printf("Total time: %.2f ms\n", all_time_ns / 1000000.0);
    printf("Cycles per yield: %llu\n", per_cycles);
    printf("Time per yield: %.2f μs\n", per_time_ns / 1000.0);
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