#include <unistd.h>
#include <stdio.h>

#define ITERATIONS 100000

// 使用约定的文件描述符
#define PIPE_READ_FD  3  // 从ping读取
#define PIPE_WRITE_FD 4  // 向ping写入

int main(int argc, char *argv[]) {
    char ping_msg;
    char pong_msg = 'O';
    int count = 0;
    
    printf("[PONG] Ready with fd: read=%d, write=%d\n", 
           PIPE_READ_FD, PIPE_WRITE_FD);
    
    // 响应第一个测试消息
    read(PIPE_READ_FD, &ping_msg, 1);
    write(PIPE_WRITE_FD, &pong_msg, 1);
    count++;
    
    printf("[PONG] Connection established, entering main loop...\n");
    
    // 主循环
    while (read(PIPE_READ_FD, &ping_msg, 1) == 1) {
        write(PIPE_WRITE_FD, &pong_msg, 1);
        count++;
        
        if (count >= ITERATIONS + 1) {
            break;
        }
    }
    
    printf("[PONG] Completed %d iterations\n", count);
    
    return 0;
}
