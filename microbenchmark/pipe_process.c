#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdint.h>
#include "cycle.h"

static void pingpong(int rd, int wr, int n) {
    char buf[2];
    for (int i = 0; i < n; i++) {
        if (read(rd, buf, 1) != 1) {
            perror("read");
            exit(1);
        }
        if (write(wr, buf, 1) != 1) {
            perror("write");
            exit(1);
        }
    }
}

int main() {
    int parent_to_child[2];
    if (pipe(parent_to_child) == -1) {
        perror("pipe");
        return 1;
    }
    int child_to_parent[2];
    if (pipe(child_to_parent) == -1) {
        perror("pipe");
        return 1;
    }

    int n = 10000;

    int child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    } else if (child == 0) {
        pingpong(parent_to_child[0], child_to_parent[1], n);
        exit(0);
    } else {
        char buf[2] = {'z'};
        if (write(parent_to_child[1], buf, 1) != 1) {
            perror("write");
            exit(1);
        }
        uint64_t start = get_cycle_count();
        pingpong(child_to_parent[0], parent_to_child[1], n);
        wait(NULL);

        uint64_t all_cycles = get_cycle_count() - start;
        uint64_t switch_cycles = all_cycles / (2 * n);
        double all_time_ns = cycle_to_time_ns(all_cycles, FPGA_HZ);
        double switch_time_ns = all_time_ns / (n * 2.0);

        printf("Iterations: %d\n", n);
        printf("Total cycles: %llu\n", all_cycles);
        printf("Total time: %.2f ms\n", all_time_ns / 1000000.0);
        printf("Cycles per switch: %llu\n", switch_cycles);
        printf("Time per switch: %.2f μs\n", switch_time_ns / 1000.0);
    }
}