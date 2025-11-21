#include <stdio.h>

int main() {
    int count = 0;

    printf("Begin running!\n");
    while (1) {
        count++;
        if (count % 10000000 == 0) {
            printf("I'm still running!\n");
        }
    }

    return 0;
}