#ifndef CYCLE_H
#define CYCLE_H

#include <stdint.h>

#define FPGA_HZ 50000000ULL // 50M HZ

static inline uint64_t get_cycle_count() {
    uint64_t cycle;
    asm volatile("rdcycle %0" : "=r"(cycle));
    return cycle;
}

static inline uint64_t cycle_to_time_ns(uint64_t cycle, uint64_t frequency_hz) {
    return (cycle * 1000000000ULL) / frequency_hz;
}

#endif // CYCLE_H