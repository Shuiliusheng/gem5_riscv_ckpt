#include "ckptinfo.h"
#include "fastlz.h"

typedef struct{
    uint64_t addr;
    uint64_t data;
}LoadInfo;
//    uint64_t size;
//first load默认是8B


typedef struct{
    uint64_t start;
    uint64_t simNum;
    uint64_t exitpc;
    uint64_t exit_cause;
}SimInfo;

void read_ckptsyscall(FILE *fp)
{
    uint64_t filesize, allocsize, alloc_vaddr, nowplace;
    nowplace = ftell(fp);
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp) - nowplace;
    fseek(fp, nowplace, SEEK_SET);

    allocsize = filesize + (4096-filesize%4096);
    alloc_vaddr = (uint64_t)mmap((void *)0x2000000, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);    
    
    fread((void *)alloc_vaddr, filesize, 1, fp);

    RunningInfo *runinfo = (RunningInfo *)&runningInfo;
    runinfo->syscall_info_addr = alloc_vaddr;
    runinfo->nowcallnum = 0;
    runinfo->totalcallnum = *((uint64_t *)alloc_vaddr);

    printf("--- step 5, read syscall infor and map to memory, totalcallnum: %d ---\n", runinfo->totalcallnum);
}


void alloc_memrange(FILE *p)
{
    MemRangeInfo memrange;
    uint64_t mrange_num=0;
    fread(&mrange_num, 8, 1, p);
    printf("--- step 3, read memory range information and do map, range num: %d ---\n", mrange_num);
    for(int i=0;i<mrange_num;i++){
        fread(&memrange, sizeof(MemRangeInfo), 1, p);
        if(memrange.size !=0){
            int* arr = static_cast<int*>(mmap((void *)memrange.addr, memrange.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_FIXED, 0, 0));
            if(memrange.addr != (uint64_t)arr)
                printf("map range: (0x%lx, 0x%lx), mapped addr: 0x%lx\n", memrange.addr, memrange.addr + memrange.size, arr);
            // assert(memrange.addr == (uint64_t)arr);  
            // printf("map range: (0x%lx, 0x%lx), mapped addr: 0x%lx\n", memrange.addr, memrange.addr + memrange.size, arr);
        }
    }
}

#define MaxCompressSize 1024*1024*30
#define ExtraDataSize   1024*6
void setFistLoad(FILE *p)
{
    uint64_t loadnum = 0;
    uint8_t *rawdata = (uint8_t *)malloc(MaxCompressSize+ExtraDataSize);
    uint8_t *compdata = (uint8_t *)malloc(MaxCompressSize+ExtraDataSize);
    uint64_t compsize, rawsize, infonum = 0, baseaddr = 0;

    uint8_t *cmap;
    uint64_t mapsize = 0, addr=0, taddr=0;
    while(1) {
        compsize = 0, infonum = 0;
        fread(&compsize, 8, 1, p);
        if(compsize==0) break;

        fread(&infonum, 8, 1, p);
        fread(compdata, 1, compsize, p);
        rawsize = fastlz_decompress(compdata, compsize, rawdata, MaxCompressSize+ExtraDataSize);
        printf("rawsize: %d, compsize: %d, infonum: %d\n", rawsize, compsize, infonum);
        baseaddr = (uint64_t)rawdata;
        for(int n=0; n<infonum; n++) {
            addr = *((uint64_t *)baseaddr);
            if(addr==0) break;
            baseaddr += 8;
            mapsize = *((uint64_t *)baseaddr) / 64;
            baseaddr+=8;
            cmap = (uint8_t *)baseaddr;
            baseaddr += mapsize;
            for(int i=0;i<mapsize;i++) {
                taddr = addr + (i*8*8);
                for(int m=0; m<8; m++) {
                    if(cmap[i]%2==1) {
                        *((uint64_t *)(taddr + m*8)) = *((uint64_t *)baseaddr);
                        baseaddr+=8;
                        loadnum++;
                    }
                    cmap[i] = cmap[i] >> 1;
                }
            }
        }
    }
    free(rawdata);
    free(compdata);
    printf("--- step 4, read first load infor and init them, first load num: %d ---\n", loadnum);
}


void read_ckptinfo(char ckptinfo[])
{
    uint64_t npc=0, temp=0;
    SimInfo siminfo;
    RunningInfo *runinfo = (RunningInfo *)&runningInfo;
    FILE *p=NULL;
    p = fopen(ckptinfo,"rb");
    if(p == NULL){
        printf("cannot open %s to read\n", ckptinfo);
        exit(1);
    }

    fread(&temp, 8, 1, p);
    fseek(p, 16*temp+8, SEEK_SET);

    fread(&siminfo, sizeof(siminfo), 1, p);
    uint64_t warmup = siminfo.simNum >> 32;
    siminfo.simNum = (siminfo.simNum << 32) >> 32;
    printf("sim slice info, start: %ld, simNum: %ld, rawLength: %ld, warmup: %ld, exitpc: 0x%lx, cause: %ld\n", siminfo.start, siminfo.simNum, siminfo.exit_cause>>2, warmup, siminfo.exitpc, siminfo.exit_cause%4);
    runinfo->exitpc = siminfo.exitpc;
    runinfo->exit_cause = siminfo.exit_cause % 4;
    uint64_t runLength = siminfo.exit_cause >> 2;

    //step 1: read npc
    fread(&npc, 8, 1, p);
    printf("--- step 1, read npc: 0x%lx ---\n", npc);


    //step 2: read integer and float registers
    fread(&program_intregs[0], 8, 32, p);
    fread(&program_fpregs[0], 8, 32, p);
    printf("--- step 2, read integer and float registers ---\n");

    //step 3: read memory range information and map these ranges
    alloc_memrange(p);

    //step 4: read first load information, and store these data to memory
    setFistLoad(p);
    
    //step5: 加载syscall的执行信息到内存中
    read_ckptsyscall(p);
    fclose(p);

    //step6: 将syscall和npc转换为jmp指令
    printf("--- step 6, transform syscall & npc to jmp insts --- 0x%lx\n", npc);
    getRangeInfo(ckptinfo);
    processJmp(npc);    

    //try to replace exit inst if syscall totalnum == 0
    if(runinfo->totalcallnum == 0 && runinfo->exitpc != 0) {
        printf("--- step 6.1, syscall is zero, replace exitinst with jmp inst ---\n");
        *((uint32_t *)runinfo->exitpc) = runinfo->exitJmpInst;
    }

    //step7: init npc and takeover_syscall addr to temp register
    printf("--- step 7, set jmp inst information to jmp to npc ---\n");
    updateJmpInst(runinfo->sysJmpinfos[0]);

    //step7: save registers data of boot program 
    printf("--- step n, save registers data of loader, set testing program registers, start testing ---\n");
    Save_ReadCkptIntRegs();
    Load_ProgramFpRegs();

    runinfo->cycles = 0;
    runinfo->insts = 0;
    runinfo->lastcycles = __csrr_cycle();
    runinfo->lastinsts = __csrr_instret();
    runinfo->startcycles = __csrr_cycle();
    runinfo->startinsts = __csrr_instret();
    
    //step8: set the testing program's register information
    Load_ProgramIntRegs();
    StartJump();
}
