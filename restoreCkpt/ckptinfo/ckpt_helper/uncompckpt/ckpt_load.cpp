#include "ckptinfo.h"

typedef struct{
    uint64_t start;
    uint64_t simNum;
    uint64_t exitpc;
    uint64_t exit_cause;
}SimInfo;

uint64_t read_ckptsyscall(FILE *fp, FILE *fq)
{
    uint64_t filesize, allocsize, alloc_vaddr, nowplace;
    nowplace = ftell(fp);
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp) - nowplace;
    fseek(fp, nowplace, SEEK_SET);

    allocsize = filesize + (4096-filesize%4096);
    alloc_vaddr = (uint64_t)mmap((void *)0x2000000, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);    
    
    fread((void *)alloc_vaddr, filesize, 1, fp);
    fwrite((void *)alloc_vaddr, filesize, 1, fq);
    return alloc_vaddr;
}


void read_ckptinfo(char ckptname[], char newckptname[])
{
    uint64_t npc=0, temp=0, numinfos=0;
    SimInfo siminfo;
    FILE *p=NULL, *q=NULL;
    p = fopen(ckptname, "rb");
    q = fopen(newckptname, "wb");
    if(p == NULL){
        printf("cannot open %s to read\n", ckptname);
        exit(1);
    }

    fread(&numinfos, 8, 1, p);
    fwrite(&numinfos, 8, 1, q);
    temp = 0;
    MemRangeInfo textinfo;
    for(int i=0;i<numinfos;i++){
        fread(&textinfo, sizeof(MemRangeInfo), 1, p);
        fwrite(&textinfo, sizeof(MemRangeInfo), 1, q);
    }

    fread(&siminfo, sizeof(siminfo), 1, p);
    fwrite(&siminfo, sizeof(siminfo), 1, q);

    fread(&npc, 8, 1, p);
    fwrite(&npc, 8, 1, q);

    uint64_t intregs[32], fpregs[32];
    fread(&intregs[0], 8, 32, p);
    fread(&fpregs[0], 8, 32, p);
    fwrite(&intregs[0], 8, 32, q);
    fwrite(&fpregs[0], 8, 32, q);

    MemRangeInfo memrange;
    uint64_t mrange_num=0;
    fread(&mrange_num, 8, 1, p);
    fwrite(&mrange_num, 8, 1, q);
    for(int i=0;i<mrange_num;i++){
        fread(&memrange, sizeof(MemRangeInfo), 1, p);
        fwrite(&memrange, sizeof(MemRangeInfo), 1, q);
    }

    //step 4: read first load information, and store these data to memory
    unzipFirstLoads(p, q);
    
    uint64_t alloc_vaddr = read_ckptsyscall(p, q);
    fclose(p);
    fclose(q);
}


