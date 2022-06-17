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
    uint64_t addr;
    uint32_t inst;
}JmpInfo;

typedef struct{
    int num;
    JmpInfo *infos;
}JmpRepInfo;

typedef struct{
    uint64_t addr;
    uint64_t size;
}MemRangeInfo;

typedef struct{
    uint64_t pc;
    uint64_t num;
    uint64_t p0, p1, p2;
    uint64_t hasret, ret;
    uint64_t bufaddr, data_offset, data_size;
}SyscallInfo;


uint64_t loadelf(char * progname);
void read_ckptinfo(char ckptinfo[]);

void setJmp(uint64_t instaddr, uint64_t base, uint64_t target);
void getRangeInfo(char filename[]);
void produceJmpInst(uint64_t npc, uint64_t exitpc, uint64_t syscall_addr);

extern MemRangeInfo text_seg;

#define TPoint1 0x100000
#define TPoint2 0x1FF000
#define MaxJALOffset 0xFFFF0
#endif
