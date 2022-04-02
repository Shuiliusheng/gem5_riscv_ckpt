#include "info.h"

void read_ckptsyscall(char filename[])
{
    FILE *fp=fopen(filename,"rb");
    uint64_t filesize, allocsize, alloc_vaddr;
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    allocsize = filesize + (4096-filesize%4096);

    alloc_vaddr = (uint64_t)mmap((void *)0x2000000, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);    
    
    fseek(fp, 0, SEEK_SET);
    if (fread((void *)alloc_vaddr, filesize, 1, fp) != 1) {
        printf("cannot read file: %s\n", filename);
        exit(1);
    }
    fclose(fp);

    RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
    runinfo->syscall_info_addr = alloc_vaddr;
    runinfo->nowcallnum = 0;
    runinfo->totalcallnum = *((uint64_t *)alloc_vaddr);

    printf("alloc_vaddr: 0x%lx, filesize: 0x%lx, allocsize: 0x%lx, syscallnum: %ld\n", alloc_vaddr, filesize, allocsize, runinfo->totalcallnum);
}




int main(int argc, char **argv)
{
    if(argc < 3){
        printf("parameters are not enough!\n");
        printf("./read ckptinfo.log ckpt_syscallinfo.log \n");
        return 0;
    }
    //alloc memory for record runtime information

    uint64_t alloc_vaddr = (uint64_t)mmap((void*)RunningInfoAddr, 4096*2, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if(alloc_vaddr != RunningInfoAddr){
        printf("cannot alloc memory in 0x%lx for record runtime information\n", RunningInfoAddr);
        return 0;
    }
    // takeoverSyscall();

    loadelf(argv[3]);
    read_ckptinfo(argv[1], argv[2]);
    return 0;
}