#include "ckptinfo.h"

uint64_t placefold[2048];
uint64_t readckpt_regs[32];
uint64_t program_intregs[32];
uint64_t program_fpregs[32];
RunningInfo runningInfo;

int main(int argc, char **argv)
{
    if(argc < 2){
        printf("parameters are not enough!\n");
        printf("./readckpt.riscv ckptinfo.log exe\n");
        return 0;
    }
    uint64_t alloc_vaddr = 0;
    printf("stack addr: 0x%lx\n", &alloc_vaddr);
    loadelf(argv[2], argv[1]);
    read_ckptinfo(argv[1]);
    return 0;
}