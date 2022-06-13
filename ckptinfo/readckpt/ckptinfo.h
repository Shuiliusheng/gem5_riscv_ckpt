#ifndef __INFO_H__
#define __INFO_H__

#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>


#define RunningInfoAddr 0x150000
#define StoreIntRegAddr "0x150028"
#define StoreFpRegAddr  "0x150128"
#define OldIntRegAddr   "0x150228"

//jmpRtemp1的二进制
#define ECall_Replace 0x00d06013
#define Cause_ExitSysCall 1
#define Cause_ExitInst 2

extern uint64_t takeOverAddr;

typedef struct{
    uint64_t exitpc;
    uint64_t exit_cause;
    uint64_t syscall_info_addr;
    uint64_t nowcallnum;
    uint64_t totalcallnum;
    uint64_t intregs[32];
    uint64_t fpregs[32];
    uint64_t oldregs[32];
    uint64_t cycles;
    uint64_t insts;
    uint64_t lastcycles;
    uint64_t lastinsts;
    uint64_t startcycles;
    uint64_t startinsts;
}RunningInfo;

typedef struct{
    uint64_t addr;
    uint64_t size;
}MemRangeInfo;


extern MemRangeInfo data_seg, text_seg;

void takeoverSyscall();
uint64_t loadelf(char * progname, char *ckptinfo);
void read_ckptsyscall(char filename[]);
void read_ckptinfo(char ckptinfo[]);

//读写临时寄存器 + 跳转临时寄存器
#define WriteRTemp(srcreg, rtempnum) "ori x0, " srcreg ", 8+" #rtempnum " \n\t"
#define ReadRTemp(dstreg, rtempnum) "ori x0, " dstreg ", 4+" #rtempnum " \n\t"
#define JmpRTemp(rtempnum) "ori x0, x0, 12+" #rtempnum " \n\t"

#define Load_necessary(BaseAddr) asm volatile( \
    "li a0, " BaseAddr " \n\t"   \
    "ld sp,8*2(a0)  \n\t"   \
    "ld gp,8*3(a0)  \n\t"   \
    "ld tp,8*4(a0)  \n\t"   \
    "ld fp,8*8(a0)  \n\t"   \
); 

#define JmpTemp(num) asm volatile( \
    JmpRTemp(num)   \
); 

#define WriteTemp(num, val) asm volatile( \
    "mv t0, %[data] \n\t"   \
    WriteRTemp("t0", num) \
    "fence \n\t"   \
    : :[data]"r"(val)  \
); 

#define Save_int_regs(BaseAddr) asm volatile( \
    WriteRTemp("a0", 2) \
    "fence \n\t"   \
    "li a0, " BaseAddr " \n\t"   \
    "sd x1,8*1(a0)  \n\t"   \
    "sd x2,8*2(a0)  \n\t"   \
    "sd x3,8*3(a0)  \n\t"   \
    "sd x4,8*4(a0)  \n\t"   \
    "sd x5,8*5(a0)  \n\t"   \
    "sd x6,8*6(a0)  \n\t"   \
    "sd x7,8*7(a0)  \n\t"   \
    "sd x8,8*8(a0)  \n\t"   \
    "sd x9,8*9(a0)  \n\t"   \
    "sd x10,8*10(a0)  \n\t"   \
    "sd x11,8*11(a0)  \n\t"   \
    "sd x12,8*12(a0)  \n\t"   \
    "sd x13,8*13(a0)  \n\t"   \
    "sd x14,8*14(a0)  \n\t"   \
    "sd x15,8*15(a0)  \n\t"   \
    "sd x16,8*16(a0)  \n\t"   \
    "sd x17,8*17(a0)  \n\t"   \
    "sd x18,8*18(a0)  \n\t"   \
    "sd x19,8*19(a0)  \n\t"   \
    "sd x20,8*20(a0)  \n\t"   \
    "sd x21,8*21(a0)  \n\t"   \
    "sd x22,8*22(a0)  \n\t"   \
    "sd x23,8*23(a0)  \n\t"   \
    "sd x24,8*24(a0)  \n\t"   \
    "sd x25,8*25(a0)  \n\t"   \
    "sd x26,8*26(a0)  \n\t"   \
    "sd x27,8*27(a0)  \n\t"   \
    "sd x28,8*28(a0)  \n\t"   \
    "sd x29,8*29(a0)  \n\t"   \
    "sd x30,8*30(a0)  \n\t"   \
    "sd x31,8*31(a0)  \n\t"   \
    ReadRTemp("a1", 2) \
    "fence  \n\t"   \
    "sd a1,8*10(a0)  #store a0  \n\t"   \
);  

#define Load_int_regs(BaseAddr) asm volatile( \
    "li a0, " BaseAddr " \n\t"   \
    "ld x1,8*1(a0)  \n\t"   \
    "ld x2,8*2(a0)  \n\t"   \
    "ld x3,8*3(a0)  \n\t"   \
    "ld x4,8*4(a0)  \n\t"   \
    "ld x5,8*5(a0)  \n\t"   \
    "ld x6,8*6(a0)  \n\t"   \
    "ld x7,8*7(a0)  \n\t"   \
    "ld x8,8*8(a0)  \n\t"   \
    "ld x9,8*9(a0)  \n\t"   \
    "ld x11,8*11(a0)  \n\t"   \
    "ld x12,8*12(a0)  \n\t"   \
    "ld x13,8*13(a0)  \n\t"   \
    "ld x14,8*14(a0)  \n\t"   \
    "ld x15,8*15(a0)  \n\t"   \
    "ld x16,8*16(a0)  \n\t"   \
    "ld x17,8*17(a0)  \n\t"   \
    "ld x18,8*18(a0)  \n\t"   \
    "ld x19,8*19(a0)  \n\t"   \
    "ld x20,8*20(a0)  \n\t"   \
    "ld x21,8*21(a0)  \n\t"   \
    "ld x22,8*22(a0)  \n\t"   \
    "ld x23,8*23(a0)  \n\t"   \
    "ld x24,8*24(a0)  \n\t"   \
    "ld x25,8*25(a0)  \n\t"   \
    "ld x26,8*26(a0)  \n\t"   \
    "ld x27,8*27(a0)  \n\t"   \
    "ld x28,8*28(a0)  \n\t"   \
    "ld x29,8*29(a0)  \n\t"   \
    "ld x30,8*30(a0)  \n\t"   \
    "ld x31,8*31(a0)  \n\t"   \
    "ld a0,8*10(a0)  \n\t"   \
); 


#define Save_fp_regs(BaseAddr) asm volatile( \
    WriteRTemp("a0", 2) \
    "fence \n\t"   \
    "li a0, " BaseAddr " \n\t"   \
    "fsd f1,8*0(a0)  \n\t"   \
    "fsd f1,8*1(a0)  \n\t"   \
    "fsd f2,8*2(a0)  \n\t"   \
    "fsd f3,8*3(a0)  \n\t"   \
    "fsd f4,8*4(a0)  \n\t"   \
    "fsd f5,8*5(a0)  \n\t"   \
    "fsd f6,8*6(a0)  \n\t"   \
    "fsd f7,8*7(a0)  \n\t"   \
    "fsd f8,8*8(a0)  \n\t"   \
    "fsd f9,8*9(a0)  \n\t"   \
    "fsd f10,8*10(a0)  \n\t"   \
    "fsd f11,8*11(a0)  \n\t"   \
    "fsd f12,8*12(a0)  \n\t"   \
    "fsd f13,8*13(a0)  \n\t"   \
    "fsd f14,8*14(a0)  \n\t"   \
    "fsd f15,8*15(a0)  \n\t"   \
    "fsd f16,8*16(a0)  \n\t"   \
    "fsd f17,8*17(a0)  \n\t"   \
    "fsd f18,8*18(a0)  \n\t"   \
    "fsd f19,8*19(a0)  \n\t"   \
    "fsd f20,8*20(a0)  \n\t"   \
    "fsd f21,8*21(a0)  \n\t"   \
    "fsd f22,8*22(a0)  \n\t"   \
    "fsd f23,8*23(a0)  \n\t"   \
    "fsd f24,8*24(a0)  \n\t"   \
    "fsd f25,8*25(a0)  \n\t"   \
    "fsd f26,8*26(a0)  \n\t"   \
    "fsd f27,8*27(a0)  \n\t"   \
    "fsd f28,8*28(a0)  \n\t"   \
    "fsd f29,8*29(a0)  \n\t"   \
    "fsd f30,8*30(a0)  \n\t"   \
    "fsd f31,8*31(a0)  \n\t"   \
    ReadRTemp("a0", 2) \
    "fence  \n\t"   \
);  

#define Load_fp_regs(BaseAddr) asm volatile( \
    WriteRTemp("a0", 2) \
    "fence \n\t"   \
    "li a0, " BaseAddr " \n\t"   \
    "fld f0,8*0(a0)  \n\t"   \
    "fld f1,8*1(a0)  \n\t"   \
    "fld f2,8*2(a0)  \n\t"   \
    "fld f3,8*3(a0)  \n\t"   \
    "fld f4,8*4(a0)  \n\t"   \
    "fld f5,8*5(a0)  \n\t"   \
    "fld f6,8*6(a0)  \n\t"   \
    "fld f7,8*7(a0)  \n\t"   \
    "fld f8,8*8(a0)  \n\t"   \
    "fld f9,8*9(a0)  \n\t"   \
    "fld f10,8*10(a0)  \n\t"   \
    "fld f11,8*11(a0)  \n\t"   \
    "fld f12,8*12(a0)  \n\t"   \
    "fld f13,8*13(a0)  \n\t"   \
    "fld f14,8*14(a0)  \n\t"   \
    "fld f15,8*15(a0)  \n\t"   \
    "fld f16,8*16(a0)  \n\t"   \
    "fld f17,8*17(a0)  \n\t"   \
    "fld f18,8*18(a0)  \n\t"   \
    "fld f19,8*19(a0)  \n\t"   \
    "fld f20,8*20(a0)  \n\t"   \
    "fld f21,8*21(a0)  \n\t"   \
    "fld f22,8*22(a0)  \n\t"   \
    "fld f23,8*23(a0)  \n\t"   \
    "fld f24,8*24(a0)  \n\t"   \
    "fld f25,8*25(a0)  \n\t"   \
    "fld f26,8*26(a0)  \n\t"   \
    "fld f27,8*27(a0)  \n\t"   \
    "fld f28,8*28(a0)  \n\t"   \
    "fld f29,8*29(a0)  \n\t"   \
    "fld f30,8*30(a0)  \n\t"   \
    "fld f31,8*31(a0)  \n\t"   \
    ReadRTemp("a0", 2) \
    "fence  \n\t"   \
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
