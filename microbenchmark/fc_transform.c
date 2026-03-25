// microbenchmark/fc_transform.c
// 函数链测试 - 数据处理者：读 fd=3，写 fd=4
// 对每字节做 XOR 0xAB 变换
#include <unistd.h>

#define CHUNK 4096

int main(void) {
    char buf[CHUNK];
    ssize_t n;
    while ((n = read(3, buf, CHUNK)) > 0) {
        for (ssize_t i = 0; i < n; i++) buf[i] ^= 0xAB;
        if (write(4, buf, n) != n) break;
    }
    return 0;
}
