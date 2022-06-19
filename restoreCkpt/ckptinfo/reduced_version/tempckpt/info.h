#ifndef __INFO_H__
#define __INFO_H__

#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#define Cause_ExitSysCall 1
#define Cause_ExitInst 2


typedef struct{
    uint64_t addr;
    uint64_t size;
}MemRangeInfo;


extern bool showload;
extern MemRangeInfo data_seg, text_seg;

uint64_t loadelf(char * progname, char *ckptinfo);
void read_ckptsyscall(char filename[]);
void read_ckptinfo(char ckptinfo[], char ckpt_sysinfo[]);


#endif
