#include "info.h"

bool showload = false;

int main(int argc, char **argv)
{
    if(argc < 4){
        printf("parameters are not enough!\n");
        printf("./read ckptinfo.log ckpt_syscallinfo.log exe.riscv showload (0/1) \n");
        return 0;
    }
    if(argc == 5){
        showload = true;
    }
    loadelf(argv[3], argv[1]);
    read_ckptinfo(argv[1], argv[2]);
    return 0;
}