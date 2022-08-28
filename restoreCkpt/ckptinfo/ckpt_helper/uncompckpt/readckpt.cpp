#include "ckptinfo.h"


int main(int argc, char **argv)
{
    if(argc < 2){
        printf("parameters are not enough!\n");
        printf("./readckpt.riscv ckptinfo.log newckpt.log exe\n");
        return 0;
    }

    read_ckptinfo(argv[1], argv[2]);
    return 0;
}