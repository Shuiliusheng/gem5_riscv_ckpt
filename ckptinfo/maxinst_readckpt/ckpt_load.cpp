#include "ckptinfo.h"
#include "ctrl.h"

typedef struct{
    uint64_t addr;
    uint64_t size;
    uint64_t data;
}LoadInfo;

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

    RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
    runinfo->syscall_info_addr = alloc_vaddr;
    runinfo->nowcallnum = 0;
    runinfo->totalcallnum = *((uint64_t *)alloc_vaddr);

    printf("--- step 5, read syscall infor and map to memory, totalcallnum: %d ---\n", runinfo->totalcallnum);
}


void alloc_memrange(FILE *p)
{
    MemRangeInfo memrange, extra;
    uint64_t mrange_num=0;
    fread(&mrange_num, 8, 1, p);
    printf("--- step 3, read memory range information and do map, range num: %d ---\n", mrange_num);
    uint64_t tsaddr = text_seg.addr, teaddr = text_seg.addr + text_seg.size;
    for(int i=0;i<mrange_num;i++){
        fread(&memrange, sizeof(MemRangeInfo), 1, p);
        printf("load range: %d\r", i);
        extra.size = 0;
        extra.addr = 0;
        uint64_t msaddr = memrange.addr, meaddr = memrange.addr + memrange.size;
        //delete the range that covered by text segment
        if(msaddr < tsaddr && meaddr > tsaddr) {
            memrange.size = tsaddr - msaddr;
            if(meaddr > teaddr) {
                extra.addr = teaddr;
                extra.size = meaddr - teaddr;
            }
        }
        else if(msaddr >= tsaddr && msaddr <= teaddr) {
            if(meaddr <= teaddr) 
                memrange.size = 0;
            else{
                memrange.addr = teaddr;
                memrange.size = meaddr - teaddr;
            }
        }

        if(memrange.size !=0){
            int* arr = static_cast<int*>(mmap((void *)memrange.addr, memrange.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_FIXED, 0, 0));
            if(memrange.addr != (uint64_t)arr)
                printf("map range: (0x%lx, 0x%lx), mapped addr: 0x%lx\n", memrange.addr, memrange.addr + memrange.size, arr);
            assert(memrange.addr == (uint64_t)arr);  
        }

        if(extra.size !=0){
            int* arr1 = static_cast<int*>(mmap((void *)extra.addr, extra.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_FIXED, 0, 0));
            if(extra.addr != (uint64_t)arr1)
                printf("map range: (0x%lx, 0x%lx), mapped addr: 0x%lx\n", extra.addr, extra.addr + extra.size, arr1);
            assert(extra.addr == (uint64_t)arr1); 
        }
    }
}

void setFistLoad(FILE *p)
{
    uint64_t loadnum = 0;
    fread(&loadnum, 8, 1, p);
    printf("--- step 4, read first load infor and init them, first load num: %d ---\n", loadnum);
    LoadInfo *linfos = (LoadInfo *)malloc(sizeof(LoadInfo)*loadnum);
    fread(&linfos[0], sizeof(LoadInfo), loadnum, p);
    for(int j=0;j<loadnum;j++){
        // printf("load %d, addr: 0x%lx, size: %d, data: 0x%lx\n", j, linfos[j].addr, linfos[j].size, linfos[j].data);
        switch(linfos[j].size) {
            case 1: *((uint8_t *)linfos[j].addr) = (uint8_t)linfos[j].data; break;
            case 2: *((uint16_t *)linfos[j].addr) = (uint16_t)linfos[j].data; break;
            case 4: *((uint32_t *)linfos[j].addr) = (uint32_t)linfos[j].data; break;
            case 8: *((uint64_t *)linfos[j].addr) = linfos[j].data; break;
        }
    }
    free(linfos);
}


void read_ckptinfo(char ckptinfo[])
{
    uint64_t npc=0, temp=0;
    SimInfo siminfo;
    RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
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
    fread(&runinfo->intregs[0], 8, 32, p);
    fread(&runinfo->fpregs[0], 8, 32, p);
    printf("--- step 2, read integer and float registers ---\n");

    //step 3: read memory range information and map these ranges
    alloc_memrange(p);

    //step 4: read first load information, and store these data to memory
    setFistLoad(p);
    
    //step5: 加载syscall的执行信息到内存中
    read_ckptsyscall(p);
    fclose(p);

    //step6: 将syscall和npc转换为jmp指令
    printf("--- step 6, transform syscall & npc to jmp insts ---\n");
    getRangeInfo(ckptinfo);
    produceJmpInst(npc);

    //try to replace exit inst if syscall totalnum == 0
    if(runinfo->totalcallnum == 0) {
        printf("--- step 6.1, syscall is zero, replace exitinst with jmp inst ---\n");
        *((uint32_t *)runinfo->exitpc) = runinfo->exitJmpInst;
    }

    //step7: init npc and takeover_syscall addr to temp register
    printf("--- step 7, set jmp inst information to jmp to npc ---\n");
    updateJmpInst(runinfo->sysJmpinfos[0]);

    //step7: save registers data of boot program 
    printf("--- step n, save registers data of loader, set testing program registers, start testing ---\n");
    Context_Operation("sd x", OldIntRegAddr);

    if(runLength != 0){
        init_start(runLength, warmup);
        printf("maxinst: %ld, warmup %%10\n", runLength);
    }
    else {
        init_start(siminfo.simNum, siminfo.simNum/20);
        printf("maxinst: %ld, warmup %%10\n", siminfo.simNum);
    }

    
    runinfo->cycles = 0;
    runinfo->insts = 0;
    runinfo->lastcycles = __csrr_cycle();
    runinfo->lastinsts = __csrr_instret();
    runinfo->startcycles = __csrr_cycle();
    runinfo->startinsts = __csrr_instret();

    //step8: set the testing program's register information
    Context_Operation("fld f", StoreFpRegAddr);
    Context_Operation("ld x", StoreIntRegAddr);

    StartJump();
}
