#include "info.h"

typedef struct{
    uint64_t addr;
    uint64_t size;
    uint64_t data;
}LoadInfo;


typedef struct{
    uint32_t addr;
    uint32_t size;
}ZeroLoadInfo;

typedef struct{
    uint32_t addr;
    uint32_t data;
    uint32_t data1;
}Load8Info;

typedef struct{
    uint32_t addr;
    uint32_t data;
}Load4Info;



typedef struct{
    uint64_t start;
    uint64_t simNum;
    uint64_t exitpc;
    uint64_t exit_cause;
}SimInfo;

void read_ckptsyscall(char filename[])
{
    uint64_t filesize, allocsize, alloc_vaddr;
    FILE *fp=NULL;
    fp = fopen(filename,"rb");
    if(fp == NULL) {
        printf("cannot open %s to read\n", filename);
        exit(1);
    }
    
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    allocsize = filesize + (4096-filesize%4096);
    alloc_vaddr = (uint64_t)mmap((void *)0x2000000, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);    

    printf("ckptsyscall file alloc_vaddr: 0x%lx\n", alloc_vaddr);
    
    if (fread((void *)alloc_vaddr, filesize, 1, fp) != 1) {
        printf("cannot read file: %s\n", filename);
        exit(1);
    }
    fclose(fp);

    RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
    runinfo->syscall_info_addr = alloc_vaddr;
    runinfo->nowcallnum = 0;
    runinfo->totalcallnum = *((uint64_t *)alloc_vaddr);

    printf("--- step 5, read syscall information and map them to memory, totalcallnum: %d ---\n", runinfo->totalcallnum);
    if(ShowLog)
        printf("syscallnum: %ld, alloc_mem: (0x%lx 0x%lx)\n", runinfo->totalcallnum, alloc_vaddr,  alloc_vaddr + allocsize);
}



void read_ckptinfo(char ckptinfo[], char ckpt_sysinfo[])
{
    uint64_t npc, mrange_num=0, loadnum = 0, temp=0;
    MemRangeInfo memrange, extra;
    LoadInfo loadinfo;
    SimInfo siminfo;
    RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
    uint64_t cycles[2], insts[2];
    cycles[0] = __csrr_cycle();
    insts[0] = __csrr_instret();

    FILE *p=NULL;
    p = fopen(ckptinfo,"rb");
    if(p == NULL){
        printf("cannot open %s to read\n", ckptinfo);
        exit(1);
    }

    fread(&temp, 8, 1, p);
    fseek(p, 16*temp+8, SEEK_SET);

    fread(&siminfo, sizeof(siminfo), 1, p);
    printf("siminfo, start: %d, simNum: %d, exitpc: 0x%lx, cause: %d\n", siminfo.start, siminfo.simNum, siminfo.exitpc, siminfo.exit_cause);
    runinfo->exitpc = siminfo.exitpc;
    runinfo->exit_cause = siminfo.exit_cause;

    //step 1: read npc
    fread(&npc, 8, 1, p);
    printf("--- step 1, read npc: 0x%lx ---\n", npc);


    //step 2: read integer and float registers
    fread(&runinfo->intregs[0], 8, 32, p);
    fread(&runinfo->fpregs[0], 8, 32, p);
    printf("--- step 2, read integer and float registers ---\n");

    //step 3: read memory range information and map these ranges
    fread(&mrange_num, 8, 1, p);
    printf("--- step 3, read memory range information and do map, range num: %d ---\n", mrange_num);
    MemRangeInfo *minfos = (MemRangeInfo *)malloc(sizeof(MemRangeInfo)*mrange_num);
    fread(&minfos[0], sizeof(MemRangeInfo), mrange_num, p);
    /*for(int i=0;i<mrange_num;i++){
        printf("load range: %d\r", i);
        memrange = minfos[i];
        extra.size = 0;
        extra.addr = 0;
        uint64_t msaddr = memrange.addr, meaddr = memrange.addr + memrange.size;
        uint64_t tsaddr = text_seg.addr, teaddr = text_seg.addr + text_seg.size;
        //delete the range that covered by text segment
        if(msaddr < tsaddr && meaddr > tsaddr) {
            memrange.size = tsaddr - msaddr;
            if(meaddr > teaddr) {
                extra.addr = teaddr;
                extra.size = meaddr - teaddr;
            }
        }
        else if(msaddr >= tsaddr && msaddr <= teaddr) {
            if(meaddr <= teaddr) {
                memrange.size = 0;
            }
            else{
                memrange.addr = teaddr;
                memrange.size = meaddr - teaddr;
            }
        }

        if(memrange.size !=0){
            if(memrange.addr < 0xfffffffff){ // a small limit
                int* arr = static_cast<int*>(mmap((void *)memrange.addr, memrange.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_FIXED, 0, 0));
                if(memrange.addr != (uint64_t)arr)
                    printf("map range: (0x%lx, 0x%lx), mapped addr: 0x%lx\n", memrange.addr, memrange.addr + memrange.size, arr);
                assert(memrange.addr == (uint64_t)arr);  
            }
        }

        if(extra.size !=0){
            int* arr1 = static_cast<int*>(mmap((void *)extra.addr, extra.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_FIXED, 0, 0));
            if(ShowLog)
                printf("map range: (0x%lx, 0x%lx), mapped addr: 0x%lx\n", extra.addr, extra.addr + extra.size, arr1);
            assert(extra.addr == (uint64_t)arr1); 
        }
    }*/

    //step 4: read first load information, and store these data to memory
    printf("--- step 4, read first load information and init these loads\n");
    uint32_t zeroloadnum, load8num, load4num, load2num, load1num;
    fread(&zeroloadnum, 4, 1, p);
    printf("--- step 4.1, read zero load, load num: %d --- : %d\n", zeroloadnum, sizeof(ZeroLoadInfo));
    ZeroLoadInfo *linfos0 = (ZeroLoadInfo *)malloc(sizeof(ZeroLoadInfo)*zeroloadnum);
    fread(&linfos0[0], sizeof(ZeroLoadInfo), 10000, p);
    for(int j=0;j<10000;j++){
        //if(j%100 == 0)
        uint64_t addr = linfos0[j].addr;
        printf("zero load %d: 0x%lx %d\n", j, addr, linfos0[j].size);
	    /*switch(linfos0[j].size) {
	        case 1: *((uint8_t *)addr) = 0; break;
            case 2: *((uint16_t *)addr) = 0; break;
	        case 4: *((uint32_t *)addr) = 0; break;
	        case 8: *((uint64_t *)addr) = 0; break;
        }*/
    }
    free(linfos0);

    /*fread(&load8num, 4, 1, p);
    printf("--- step 4.2, read load 8 bytes, load num: %d --- %d\n", load8num, sizeof(Load8Info));
    Load8Info *linfos8 = (Load8Info *)malloc(sizeof(Load8Info)*load8num);
    fread(&linfos8[0], sizeof(Load8Info), load8num, p);
    for(int j=0;j<load8num;j++){
        //if(j%100 == 0) printf("load8 %d \r", j);
        uint64_t addr = linfos8[j].addr;
	    *((uint64_t *)addr) = linfos8[j].data + ((uint64_t)linfos8[j].data1 << 32);
    }
    free(linfos8);

    fread(&load4num, 4, 1, p);
    printf("--- step 4.3, read load 4 bytes, load num: %d --- %d\n", load4num, sizeof(Load4Info));
    Load4Info *linfos4 = (Load4Info *)malloc(sizeof(Load4Info)*load4num);
    fread(&linfos4[0], sizeof(Load4Info), load4num, p);
    for(int j=0;j<load4num;j++){
        //if(j%100 == 0) printf("load4 %d \r", j);
        uint64_t addr = linfos4[j].addr;
	    *((uint32_t *)addr) = linfos4[j].data;
    }
    free(linfos4);

    fread(&load2num, 4, 1, p);
    printf("--- step 4.4, read load 2 bytes, load num: %d ---\n", load2num);
    Load4Info *linfos2 = (Load4Info *)malloc(sizeof(Load4Info)*load2num);
    fread(&linfos2[0], sizeof(Load4Info), load2num, p);
    for(int j=0;j<load2num;j++){
        //if(j%100 == 0) printf("load2 %d \r", j);
        uint64_t addr = linfos2[j].addr;
	    *((uint16_t *)addr) = (uint16_t)linfos2[j].data;
    }
    free(linfos2);


    fread(&load1num, 4, 1, p);
    printf("--- step 4.5, read load 1 bytes, load num: %d ---\n", load1num);
    Load4Info *linfos1 = (Load4Info *)malloc(sizeof(Load4Info)*load1num);
    fread(&linfos1[0], sizeof(Load4Info), load1num, p);
    for(int j=0;j<load1num;j++){
        //if(j%100 == 0) printf("load1 %d \r", j);
        uint64_t addr = linfos1[j].addr;
	    *((uint8_t *)addr) = (uint8_t)linfos1[j].data;
    }
    free(linfos1);
    
    //step5: 加载syscall的执行信息到内存中
    read_ckptsyscall(ckpt_sysinfo);

    //try to replace exit inst if syscall totalnum == 0
    if(runinfo->totalcallnum == 0 && runinfo->exit_cause == Cause_ExitInst) {
        printf("--- step 5.1, syscall is zero, replace exitinst with jmp rtemp ---\n");
        *((uint16_t *)runinfo->exitpc) = (ECall_Replace)%65536;
        *((uint16_t *)(runinfo->exitpc+2)) = (ECall_Replace) >> 16;
    }

    cycles[1] = __csrr_cycle();
    insts[1] = __csrr_instret();
    printf("load ckpt running info, cycles: %ld, insts: %ld\n", cycles[1]-cycles[0], insts[1]-insts[0]);

    //step6: init npc and takeover_syscall addr to temp register
    printf("--- step 6, init npc to rtemp(5) and takeover_syscall addr to rtemp(6) ---\n");
    WriteTemp("0", npc);
    uint64_t takeover_addr = ShowLog ? TakeOverAddrTrue : TakeOverAddrFalse;
    WriteTemp("1", takeover_addr);

    //step7: save registers data of boot program 
    printf("--- step 789, save registers data of boot program, set testing program registers, start testing ---\n");
    Save_int_regs(OldIntRegAddr);

    runinfo->cycles = 0;
    runinfo->insts = 0;
    runinfo->lastcycles = __csrr_cycle();
    runinfo->lastinsts = __csrr_instret();
    runinfo->startcycles = __csrr_cycle();
    runinfo->startinsts = __csrr_instret();

    //step8: set the testing program's register information
    Load_regs(StoreIntRegAddr);

    //step9: start the testing program
    JmpTemp("0");*/
}
