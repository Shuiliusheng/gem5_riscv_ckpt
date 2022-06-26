#include "ckptinfo.h"


int main(int argc, char **argv)
{
    if(argc < 2){
        printf("parameters are not enough!\n");
        printf("./readckpt.riscv ckptinfo.log exe\n");
        return 0;
    }

    uint64_t alloc_vaddr = (uint64_t)mmap((void*)RunningInfoAddr, 4096*2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
    if(alloc_vaddr != RunningInfoAddr){
        printf("cannot alloc memory in 0x%lx for record runtime information, alloc: 0x%lx\n", RunningInfoAddr, alloc_vaddr);
        return 0;
    }
    printf("stack addr: 0x%lx\n", &alloc_vaddr);
    loadelf(argv[2], argv[1]);

    int setwarmup = 0;
    printf("argc: %d\n", argc);
    if(argc == 4) {
        sscanf(argv[3], "%d", &setwarmup);
    }
    read_ckptinfo(argv[1], setwarmup);
    return 0;
}