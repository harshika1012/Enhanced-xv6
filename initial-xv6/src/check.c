#include <stdio.h>

int main() {
    int num = 65536;  // The input number
    int shifts = 0;   // Count of shifts

    // Keep right-shifting the number until it becomes 1
    while (num > 1) {
        num = num >> 1;
        shifts++;
    }

    printf("The number of left shifts to get 32768 is: %d\n", shifts);
    return 0;
}
