// 极简 wc：统计 stdin 的行/字/字节数
#include <stdio.h>

int main(void) {
    long lines = 0, words = 0, bytes = 0;
    int c, in_word = 0;
    while ((c = getchar()) != EOF) {
        bytes++;
        if (c == '\n') lines++;
        if (c == ' ' || c == '\t' || c == '\n') in_word = 0;
        else if (!in_word) { in_word = 1; words++; }
    }
    printf("%ld %ld %ld\n", lines, words, bytes);
    return 0;
}
