#ifndef __INFO_H__
#define __INFO_H__

#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include "define.h"

typedef struct{
    uint64_t addr;
    uint64_t size;
}MemRangeInfo;

typedef struct{
    uint64_t pc;
    uint64_t num;
    uint64_t p0;
    uint64_t ret;
    uint64_t data_offset;
}SyscallInfo;

typedef struct{
    uint64_t exitpc;
    uint64_t exit_cause;
    uint64_t syscall_info_addr;
    uint64_t nowcallnum;
    uint64_t totalcallnum;

    //perf counter information
    uint64_t cycles;
    uint64_t insts;
    uint64_t lastcycles;
    uint64_t lastinsts;
    uint64_t startcycles;
    uint64_t startinsts;
}RunningInfo;

#define Cause_ExitSysCall 1
#define Cause_ExitInst 2

uint64_t loadelf(char * progname, char *ckptinfo);
void read_ckptinfo(char ckptinfo[]);
void takeoverSyscall();
void recovery_FirstLoad(FILE *p);

extern uint64_t takeOverAddr;
extern MemRangeInfo text_seg;
extern RunningInfo runningInfo;
extern uint64_t placefold[2048];
extern uint64_t readckpt_regs[32];
extern uint64_t program_intregs[32];
extern uint64_t program_fpregs[32];
extern uint32_t JmpRTemp3Inst;  //jmp rtemp3的指令

#define Save_ProgramIntRegs() asm volatile( \
    "la a0, program_intregs  \n\t"  \
    Regs_Operations("sd x")       \
);  

#define Load_ProgramIntRegs() asm volatile( \
    "la a0, program_intregs  \n\t"      \
    Regs_Operations("ld x")           \
    "ld a0,8*10(a0) # restore a0 \n\t"   \
); 

#define Save_ProgramFpRegs() asm volatile( \
    "la a0, program_fpregs  \n\t"   \
    Regs_Operations("fsd f")       \
    "fsd f10,8*10(a0) # restore a0 \n\t"   \
);  

#define Load_ProgramFpRegs() asm volatile( \
    "la a0, program_fpregs  \n\t"  \
    Regs_Operations("fld f")      \
    "fld f10,8*10(a0) # restore a0 \n\t"   \
); 

#define Save_ReadCkptIntRegs() asm volatile( \
    "la a0, readckpt_regs  \n\t"  \
    Regs_Operations("sd x")      \
);  

#define Load_ReadCkptIntRegs() asm volatile( \
    "la a0, readckpt_regs \n\t"   \
    "ld ra,8*1(a0)  \n\t"   \
    "ld sp,8*2(a0)  \n\t"   \
    "ld gp,8*3(a0)  \n\t"   \
    "ld tp,8*4(a0)  \n\t"   \
    "ld fp,8*8(a0)  \n\t"   \
); 

#endif
