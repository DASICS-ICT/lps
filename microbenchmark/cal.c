#include <math.h>
#include <stdio.h>

double result;

int main() {
    result = 0;
    for (int i = 1; i <= 100000; i++) {
        result += sin(i) * cos(i) / (i + 1);
    }
    return 0;
}