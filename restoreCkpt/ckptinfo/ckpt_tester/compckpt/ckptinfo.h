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
    uint64_t size;
}MemRangeInfo;

typedef struct{
    uint64_t addr;
    uint64_t data;
}LoadInfo;
//默认均为8B

void read_ckptinfo(char ckptinfo[]);
void processFirstLoad(LoadInfo *linfos, uint64_t loadnum, uint64_t maxcon, FILE *q);
void processFirstLoad1(LoadInfo *linfos, uint64_t loadnum, uint64_t maxsize, FILE *q);
void processFirstLoad2(LoadInfo *linfos, uint64_t loadnum, uint64_t maxsize, FILE *q);
#endif
