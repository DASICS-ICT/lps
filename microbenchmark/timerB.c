#include <stdio.h>

const char *timer = "B";

int main() {
    int count = 0;

    printf("Begin running %s!\n", timer);
    while (1) {
        count++;
        if (count % 10000000 == 0) {
            printf("%s: I'm still running!\n", timer);
        }
    }

    return 0;
}