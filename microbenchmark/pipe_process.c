#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
// #include <linux/time.h>

static inline long long unsigned time_ns() {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts)) {
        exit(1);
    }
    return ((long long unsigned)ts.tv_sec) * 1000000000LLU +
        (long long unsigned)ts.tv_nsec;
}

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

    int n = 100000;

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
        const long long unsigned t1 = time_ns();
        pingpong(child_to_parent[0], parent_to_child[1], n);
        wait(NULL);
        const long long unsigned elapsed = time_ns() - t1;
        printf("%.3fns/ctxswitch\n", elapsed / (float) (n * 2));
    }
}