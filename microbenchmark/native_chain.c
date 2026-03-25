// microbenchmark/native_chain.c
// Native 三级管道包装器：fork 三进程，建立 Unix pipe 连接，rdcycle 测总时间
// 用法：native_chain <source-elf> <transform-elf> <sink-elf>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdint.h>
#include "cycle.h"

// 期望的管道缓冲区大小（字节），必须是页大小的倍数，且 ≤ /proc/sys/fs/pipe-max-size
// 默认 Linux pipe 大小为 65536 (64KB)，这里可按需调整
#ifndef DESIRED_PIPE_SIZE
#define DESIRED_PIPE_SIZE  (64 * 1024)   // 64 KB
#endif

/**
 * 设置管道 fd 的缓冲区大小，并打印实际生效的大小。
 * @param fd        管道的任意一端（读端或写端均可）
 * @param size      期望大小（字节）
 * @param label     用于日志打印的标签
 * @return          实际生效的大小，失败返回 -1
 */
static int set_and_print_pipe_size(int fd, int size, const char *label) {
    int ret = fcntl(fd, F_SETPIPE_SZ, size);
    if (ret < 0) {
        perror("fcntl F_SETPIPE_SZ");
        // 即使设置失败，也尝试读取当前大小
    }

    int actual = fcntl(fd, F_GETPIPE_SZ);
    if (actual < 0) {
        perror("fcntl F_GETPIPE_SZ");
        return -1;
    }

    fprintf(stderr, "[native-chain] %s pipe size: requested %d bytes, actual %d bytes (%d KB)\n",
            label, size, actual, actual / 1024);
    return actual;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <source-elf> <transform-elf> <sink-elf>\n", argv[0]);
        return 1;
    }

    char *src_elf = argv[1];
    char *xfm_elf = argv[2];
    char *snk_elf = argv[3];

    // 建立两个管道：pipe1 连接 source→transform，pipe2 连接 transform→sink
    int pipe1[2], pipe2[2];
    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        perror("pipe");
        return 1;
    }

    // --- 设置管道缓冲区大小并打印 ---
    set_and_print_pipe_size(pipe1[1], DESIRED_PIPE_SIZE, "pipe1 (source→transform)");
    set_and_print_pipe_size(pipe2[1], DESIRED_PIPE_SIZE, "pipe2 (transform→sink)");

    // 同时打印系统默认的 pipe-max-size（如果可读）
    FILE *fp = fopen("/proc/sys/fs/pipe-max-size", "r");
    if (fp) {
        int max_size = 0;
        if (fscanf(fp, "%d", &max_size) == 1) {
            fprintf(stderr, "[native-chain] system pipe-max-size: %d bytes (%d KB)\n",
                    max_size, max_size / 1024);
        }
        fclose(fp);
    }

    uint64_t t0 = get_cycle_count();

    // Fork source：写 stdout → pipe1[1]
    pid_t src_pid = fork();
    if (src_pid == 0) {
        close(pipe1[0]);
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        execl(src_elf, src_elf, NULL);
        perror("execl source");
        exit(1);
    }

    // Fork transform：读 stdin ← pipe1[0]，写 stdout → pipe2[1]
    pid_t xfm_pid = fork();
    if (xfm_pid == 0) {
        close(pipe1[1]);
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        close(pipe2[0]);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);
        execl(xfm_elf, xfm_elf, NULL);
        perror("execl transform");
        exit(1);
    }

    // Fork sink：读 stdin ← pipe2[0]
    pid_t snk_pid = fork();
    if (snk_pid == 0) {
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[1]);
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        execl(snk_elf, snk_elf, NULL);
        perror("execl sink");
        exit(1);
    }

    // 父进程关闭所有管道端，等待三个子进程
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    waitpid(src_pid, NULL, 0);
    waitpid(xfm_pid, NULL, 0);
    waitpid(snk_pid, NULL, 0);

    uint64_t t1 = get_cycle_count();
    double ms = (double)cycle_to_time_ns(t1 - t0, FPGA_HZ) / 1e6;
    fprintf(stderr, "[native-chain] total: %llu cycles  %.1f ms\n",
            (unsigned long long)(t1 - t0), ms);
    return 0;
}