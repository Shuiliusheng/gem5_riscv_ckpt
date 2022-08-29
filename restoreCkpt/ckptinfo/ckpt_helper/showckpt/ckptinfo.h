#ifndef __INFO_H__
#define __INFO_H__

#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include<vector>
using namespace std;

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

typedef struct{
    uint64_t addr;
    uint64_t data;
}LoadInfo;

void read_ckptinfo(char ckptinfo[]);
void read_FirstLoad(FILE *p, vector<LoadInfo> &tempinfos);

extern MemRangeInfo text_seg;

#endif
