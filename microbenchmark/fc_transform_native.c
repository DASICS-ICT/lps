// microbenchmark/fc_transform_native.c
// Native 对照 - 数据处理者，读 stdin，写 stdout，XOR 0xAB
#include <unistd.h>

#define CHUNK 4096

int main(void) {
    char buf[CHUNK];
    ssize_t n;
    while ((n = read(STDIN_FILENO, buf, CHUNK)) > 0) {
        for (ssize_t i = 0; i < n; i++) buf[i] ^= 0xAB;
        if (write(STDOUT_FILENO, buf, n) != n) break;
    }
    return 0;
}
