#include "ckptinfo.h"

extern uint64_t __takeOverSys_Addr;
uint64_t takeOverAddr = (uint64_t)&__takeOverSys_Addr;
uint64_t counters[16];

#define SetProcTag(tag) asm volatile( \
    "mv t0, %[value]    \n\t"  \
    Set_ProcTag("t0")  \
    : :[value]"r"(tag)  \
); 


void takeoverSyscall()
{
    asm volatile("__takeOverSys_Addr: ");
    Save_ProgramIntRegs();
    Load_ReadCkptIntRegs();
    uint64_t tag = 0;
    SetProcTag(tag);

    RunningInfo *runinfo = (RunningInfo *)&runningInfo;
    uint64_t cycles = 0, insts = 0;
    cycles = read_csr_cycle();
    insts = read_csr_instret();
    runinfo->cycles += (cycles - runinfo->lastcycles);
    runinfo->insts += (insts - runinfo->lastinsts);
    
    if (runinfo->nowcallnum >= runinfo->totalcallnum){
        printf("syscall is overflow, exit.\n");
        printf("test program user running info, cycles: %ld, insts: %ld\n", runinfo->cycles, runinfo->insts);
        printf("test program total running info, cycles: %ld, insts: %ld\n", cycles - runinfo->startcycles, insts - runinfo->startinsts);
        exit(0);
    }

    uint64_t infoaddr = runinfo->syscall_info_addr + 8 + runinfo->totalcallnum*4;
    uint32_t *sysidxs = (uint32_t *)(runinfo->syscall_info_addr + 8);
    SyscallInfo *infos = (SyscallInfo *)(infoaddr + sysidxs[runinfo->nowcallnum]*sizeof(SyscallInfo));
    program_intregs[10] = infos->p0;

    uint64_t sysnum = infos->num >> 32;
    if(program_intregs[17] != sysnum){
        printf("syscall num is wrong! callnum: 0x%lx, recorded num: 0x%lx, 0x%lx\n", program_intregs[17], infos->pc, sysnum);
        exit(0);
    }

    //this syscall has data to process
    uint64_t data_offset = infos->data_offset >> 8;
    if (data_offset != 0xffffffff){
        uint64_t data_size = (infos->num<<32) >> 32;
        uint64_t bufaddr = *(uint64_t *)(runinfo->syscall_info_addr + data_offset);
        if(bufaddr != 0) {   //these syscall also need to write data to bufaddr
            unsigned char *dst = (unsigned char *)bufaddr;
            unsigned char *data = (unsigned char *)(runinfo->syscall_info_addr + data_offset + 8);
            for(int c=0;c<data_size;c++){
                dst[c] = data[c];
            }
        }
    }
    uint64_t hasret = infos->data_offset % 256;
    if(hasret) {
        program_intregs[10] = infos->ret;
    }

    runinfo->nowcallnum ++;

    if(runinfo->nowcallnum == runinfo->totalcallnum && runinfo->exit_cause == Cause_ExitInst && runinfo->exitpc!=0) {
        printf("--- exit the last syscall %d, and replace exit pc (0x%lx) with jmp3 ---\n", runinfo->nowcallnum, runinfo->exitpc);
        *((uint32_t *)runinfo->exitpc) = JmpRTemp3Inst;
	    asm volatile("fence.i  ");
    }

    uint64_t npc = infos->pc + 4;
    SetTempReg(npc, 2);

    runinfo->lastcycles = read_csr_cycle();
    runinfo->lastinsts = read_csr_instret();

    tag = 0x1234567;
    SetProcTag(tag);

    Load_ProgramIntRegs();
    JmpTempReg(2);
}
