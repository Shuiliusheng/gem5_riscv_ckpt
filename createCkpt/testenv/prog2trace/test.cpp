#include <stdio.h>
#include <stdint.h>
// #include"define.h"

uint64_t tempStackMem[40960];

int main()
{
    for(int i=0; i<40960; i++) {
        tempStackMem[i] = i % 4;
    }

    printf("step 1\n");

    for(int i=1; i<40960; i++) {
        tempStackMem[i] = tempStackMem[i-1]+1;
    }

    printf("step 2\n");

    return 0;
}