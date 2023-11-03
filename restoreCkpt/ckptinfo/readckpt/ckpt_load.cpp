#include "ckptinfo.h"
#include "ctrl.h"
#include "fastlz.h"

extern uint64_t _JmpRTemp3Inst;
uint32_t JmpRTemp3Inst = *((uint32_t *)&_JmpRTemp3Inst);
uint64_t placefold1[2048];

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
    fclose(fp);

    RunningInfo *runinfo = (RunningInfo *)&runningInfo;
    runinfo->syscall_info_addr = alloc_vaddr;
    runinfo->nowcallnum = 0;
    runinfo->totalcallnum = *((uint64_t *)alloc_vaddr);

    printf("--- step 5, read syscall infor and map to memory, totalcallnum: %d ---\n", runinfo->totalcallnum);

    //将ecall指令替换为jmp rtemp
    SyscallInfo *sinfos = NULL;
    uint64_t infoaddr = runinfo->syscall_info_addr + 8 + runinfo->totalcallnum*4;
    uint32_t *sysidxs = (uint32_t *)(runinfo->syscall_info_addr + 8);

    int diffcallnum = -1;
    for(int i=0; i<runinfo->totalcallnum; i++) {
        if((int)sysidxs[i] > diffcallnum) 
            diffcallnum = sysidxs[i];
    }
    for(int i=0; i<diffcallnum+1; i++) {
        sinfos = (SyscallInfo *)(infoaddr + i*sizeof(SyscallInfo));
        *((uint32_t *)sinfos->pc) = JmpRTemp3Inst;
    }
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
            memrange.size = memrange.size + 4096;
            int* arr = static_cast<int*>(mmap((void *)memrange.addr, memrange.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_FIXED, 0, 0));
            if(memrange.addr != (uint64_t)arr)
                printf("map range: (0x%lx, 0x%lx), mapped addr: 0x%lx\n", memrange.addr, memrange.addr + memrange.size, arr);
            assert(memrange.addr == (uint64_t)arr);  
        }
    }
}

void read_ckptinfo(char ckptinfo[])
{
    uint64_t npc, mrange_num=0, loadnum = 0, temp=0;
    MemRangeInfo memrange, extra;
    LoadInfo loadinfo;
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
    uint64_t runLength = siminfo.exit_cause >> 2;
    siminfo.simNum = (siminfo.simNum << 32) >> 32;
    runinfo->exit_cause = siminfo.exit_cause % 4;
    runinfo->exitpc = siminfo.exitpc;

    printf("sim slice info, start: %ld, simNum: %ld, rawLength: %ld, warmup: %ld, exitpc: 0x%lx, cause: %ld\n", siminfo.start, siminfo.simNum, runLength, warmup, siminfo.exitpc, runinfo->exit_cause);
    
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
    recovery_FirstLoad(p);
    
    //step5: 加载syscall的执行信息到内存中
    read_ckptsyscall(p);

    //try to replace exit inst if syscall totalnum == 0
    if(runinfo->totalcallnum == 0 && runinfo->exit_cause == Cause_ExitInst && runinfo->exitpc!=0) {
        printf("--- step 5.1, syscall is zero, replace exitinst with jmp rtemp3 ---: 0x%lx\n", JmpRTemp3Inst);
        *((uint32_t *)runinfo->exitpc) = JmpRTemp3Inst; //(ECall_Replace)
    }

    //step6: init npc and takeover_syscall addr to temp register
    printf("--- step 6, init npc to rtemp(5) and takeover_syscall addr to rtemp(6) ---\n");
    SetTempReg(npc, 2);
    SetTempReg(takeOverAddr, 3);

    //step7: save registers data of boot program 
    printf("--- step 789, save registers data of boot program, set testing program registers, start testing ---\n");
    Save_ReadCkptIntRegs();
    Load_ProgramFpRegs();

    runinfo->cycles = 0;
    runinfo->insts = 0;
    runinfo->lastcycles = read_csr_cycle();
    runinfo->lastinsts = read_csr_instret();
    runinfo->startcycles = read_csr_cycle();
    runinfo->startinsts = read_csr_instret();
    
    if(runLength != 0){
        init_start(runLength, warmup, 1, ckptinfo);
    }
    else {
        warmup = siminfo.simNum/20;
        runLength = siminfo.simNum - warmup;
        init_start(runLength, warmup, 1, ckptinfo);
    }
    
    //step8: set the testing program's register information
    Load_ProgramIntRegs();

    //step9: start the testing program
    JmpTempReg(2);

    asm volatile("_JmpRTemp3Inst: ");
    JmpTempReg(3);
}
