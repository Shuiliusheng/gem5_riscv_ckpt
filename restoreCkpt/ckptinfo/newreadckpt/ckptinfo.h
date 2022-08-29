#ifndef __INFO_H__
#define __INFO_H__

#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>


typedef struct{
    uint64_t jalr_target;
    uint64_t jal_addr;
    uint32_t jal_inst;
}JmpRepInfo;

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
    uint32_t exitJmpInst;
    JmpRepInfo *sysJmpinfos;

    //perf counter information
    uint64_t cycles;
    uint64_t insts;
    uint64_t lastcycles;
    uint64_t lastinsts;
    uint64_t startcycles;
    uint64_t startinsts;
}RunningInfo;

void takeoverSyscall();
uint64_t loadelf(char * progname, char *ckptinfo);
void read_ckptinfo(char ckptinfo[]);

bool setJmp(uint64_t instaddr, uint64_t target);
void getRangeInfo(char filename[]);
void processJmp(uint64_t npc);
void updateJmpInst(JmpRepInfo &info);
void recovery_FirstLoad(FILE *p);

extern uint64_t takeOverAddr;
extern MemRangeInfo text_seg;
extern RunningInfo runningInfo;
extern uint64_t readckpt_regs[32];
extern uint64_t program_intregs[32];
extern uint64_t program_fpregs[32];
extern uint64_t tempMemory[2];

#define MaxJALOffset 0xFFFF0

#define StartJump() asm volatile( \
    "la a0, tempMemory  \n\t"   \
    "ld a0, 0(a0)  \n\t"        \
    "jalr x0, 0(a0)  \n\t"      \
); 

#define Context_Operation(Op) \
    Op "1,8*1(a0)  \n\t"   \
    Op "2,8*2(a0)  \n\t"   \
    Op "3,8*3(a0)  \n\t"   \
    Op "4,8*4(a0)  \n\t"   \
    Op "5,8*5(a0)  \n\t"   \
    Op "6,8*6(a0)  \n\t"   \
    Op "7,8*7(a0)  \n\t"   \
    Op "8,8*8(a0)  \n\t"   \
    Op "9,8*9(a0)  \n\t"   \
    Op "11,8*11(a0)  \n\t"   \
    Op "12,8*12(a0)  \n\t"   \
    Op "13,8*13(a0)  \n\t"   \
    Op "14,8*14(a0)  \n\t"   \
    Op "15,8*15(a0)  \n\t"   \
    Op "16,8*16(a0)  \n\t"   \
    Op "17,8*17(a0)  \n\t"   \
    Op "18,8*18(a0)  \n\t"   \
    Op "19,8*19(a0)  \n\t"   \
    Op "20,8*20(a0)  \n\t"   \
    Op "21,8*21(a0)  \n\t"   \
    Op "22,8*22(a0)  \n\t"   \
    Op "23,8*23(a0)  \n\t"   \
    Op "24,8*24(a0)  \n\t"   \
    Op "25,8*25(a0)  \n\t"   \
    Op "26,8*26(a0)  \n\t"   \
    Op "27,8*27(a0)  \n\t"   \
    Op "28,8*28(a0)  \n\t"   \
    Op "29,8*29(a0)  \n\t"   \
    Op "30,8*30(a0)  \n\t"   \
    Op "31,8*31(a0)  \n\t"   

#define Save_ProgramIntRegs() asm volatile( \
    "la a0, program_intregs  \n\t"  \
    Context_Operation("sd x")       \
);  

#define Load_ProgramIntRegs() asm volatile( \
    "la a0, program_intregs  \n\t"      \
    Context_Operation("ld x")           \
    "ld a0,8*10(a0) # restore a0 \n\t"   \
); 

#define Save_ProgramFpRegs() asm volatile( \
    "la a0, program_fpregs  \n\t"   \
    Context_Operation("fsd f")       \
    "fsd f10,8*10(a0) # restore a0 \n\t"   \
);  

#define Load_ProgramFpRegs() asm volatile( \
    "la a0, program_fpregs  \n\t"  \
    Context_Operation("fld f")      \
    "fld f10,8*10(a0) # restore a0 \n\t"   \
); 

#define Save_ReadCkptIntRegs() asm volatile( \
    "la a0, readckpt_regs  \n\t"  \
    Context_Operation("sd x")      \
);  

#define Load_ReadCkptIntRegs() asm volatile( \
    "la a0, readckpt_regs \n\t"   \
    "ld ra,8*1(a0)  \n\t"   \
    "ld sp,8*2(a0)  \n\t"   \
    "ld gp,8*3(a0)  \n\t"   \
    "ld tp,8*4(a0)  \n\t"   \
    "ld fp,8*8(a0)  \n\t"   \
); 


#define DEFINE_CSRR(s)                     \
    static inline uint64_t __csrr_##s()    \
    {                                      \
        uint64_t value;                    \
        __asm__ volatile("csrr    %0, " #s \
                         : "=r"(value)     \
                         :);               \
        return value;                      \
    }

DEFINE_CSRR(cycle)
DEFINE_CSRR(instret)

#endif
