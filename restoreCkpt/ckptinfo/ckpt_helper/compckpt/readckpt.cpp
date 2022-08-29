#include "ckptinfo.h"
#include <string.h>

/*
    choice = 1: only compress the first load with data map
    choice = 2: compress the data map with fastlz level 1
    choice = 3: compress the data map with fastlz level 2
*/
int compress_choice = 2;

int main(int argc, char **argv)
{
    if(argc < 3){
        printf("parameters are not enough!\n");
        printf("./readckpt.riscv compresslevel ckptinfo.log newckptinfo.log\n");
        printf("compresslevel: default 2\n");
        printf("\t  0: no compress\n");
        printf("\t  1: only compress the first load with data map\n");
        printf("\t  2: compress the data map with fastlz level 1\n");
        printf("\t  3: compress the data map with fastlz level 2\n");
        return 0;
    }

    compress_choice = 2;
    if(argc == 4){
        sscanf(argv[1], "%d", &compress_choice);
        printf("compress %s to %s, with the compress level %d\n", argv[2], argv[3], compress_choice);
        read_ckptinfo(argv[2], argv[3]);
    }
    else{
        printf("compress %s to %s, with the compress level %d\n", argv[1], argv[2], compress_choice);
        read_ckptinfo(argv[1], argv[2]);
    }
    return 0;
}