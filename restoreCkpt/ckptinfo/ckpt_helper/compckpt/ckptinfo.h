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
    uint64_t addr;
    uint64_t data;
}LoadInfo;
//默认均为8B

void read_ckptinfo(char ckptname[], char newckptname[]);
void compress_FirstLoad(vector<LoadInfo> &linfos, FILE *q);
void recovery_FirstLoad(FILE *p);
void read_FirstLoad(FILE *p, vector<LoadInfo> &tempinfos);

extern int compress_choice;
#endif
