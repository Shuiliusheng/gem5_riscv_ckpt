#ifndef __INFO_H__
#define __INFO_H__

#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#ifdef SHOWLOG
    #define ShowLog true
#else
    #define ShowLog false
#endif

#define RunningInfoAddr 0x150000
#define StoreIntRegAddr "0x150018"
#define OldIntRegAddr   "0x150218"

#define TakeOverAddrFalse 0x10662
#define TakeOverAddrTrue 0x10666

typedef struct{
    uint64_t syscall_info_addr;
    uint64_t nowcallnum;
    uint64_t totalcallnum;
    uint64_t intregs[32];
    uint64_t fpregs[32];
    uint64_t oldregs[32];
}RunningInfo;

typedef struct{
    uint64_t addr;
    uint64_t size;
}MemRangeInfo;

extern MemRangeInfo data_seg, text_seg;


void takeoverSyscall();
uint64_t loadelf(char * progname);
void read_ckptsyscall(char filename[]);
void read_ckptinfo(char ckptinfo[], char ckpt_sysinfo[]);


#define Save_int_regs(BaseAddr) asm volatile( \
    "sub x0, a0, x7 #save a0 to rtemp7 \n\t"   \
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
    "add x0, a1, x7  #read rtemp7 to a1  \n\t"   \
    "fence  \n\t"   \
    "sd a1,8*10(a0)  #read rtemp7 to a1  \n\t"   \
);  

#define Load_regs(BaseAddr) asm volatile( \
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


#define Load_necessary(BaseAddr) asm volatile( \
    "li a0, " BaseAddr " \n\t"   \
    "ld sp,8*2(a0)  \n\t"   \
    "ld gp,8*3(a0)  \n\t"   \
    "ld tp,8*4(a0)  \n\t"   \
    "ld fp,8*8(a0)  \n\t"   \
); 

#define JmpTemp(num) asm volatile( \
    "or x0, x0, x" num " \n\t"   \
); 

#define WriteTemp(num, val) asm volatile( \
    "mv t0, %[data] \n\t"   \
    "sub x0, t0, x" num " \n\t"   \
    "fence \n\t"   \
    :  \
    :[data]"r"(val)  \
); 


#endif
