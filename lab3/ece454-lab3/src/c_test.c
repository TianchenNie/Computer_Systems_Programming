#include <stdio.h>
#include <stdlib.h>

#define LOG2(num) (63 - __builtin_clzll((num)))
#define TWOINDEX(num) (LOG2(((num) - 1360) >> 4))
#define POW2(n) ((unsigned long int) 1 << (n))
int main(void) {
    unsigned long long int num = 1376 + 16 * ((long int) 1 << 33);
    int floor_log = LOG2(((num) - 1360) >> 4);
    size_t upper_bound_size = 1360 + 16 * POW2(floor_log);
    printf("%d\n", num > upper_bound_size);
}