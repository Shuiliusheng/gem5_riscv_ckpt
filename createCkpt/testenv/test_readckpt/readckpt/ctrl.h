#ifndef   _PFC_ASM_  
#define   _PFC_ASM_  

#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include <fcntl.h>
#include"define.h"


uint64_t placefolds[1024];
uint64_t tempStackMem[4096];
uint64_t hpcounters[64];
uint64_t procTag=0x1234567;
uint64_t maxevent=10000, warmupinst=1000, eventsel=0, maxperiod=0;
uint64_t sampleHapTimes=0;
extern uint64_t __sampleFucAddr;

uint64_t exitpc=0;
unsigned int logfile;
char str[256];
uint64_t startcycle = 0, endcycle = 0;
uint64_t startinst = 0, endinst = 0;


void sample_func();
void read_counters();
void open_logfile(char *ckptname);
void init_start(uint64_t max_inst, uint64_t warmup_inst, uint64_t max_period, char *ckptname);

void open_logfile(char *ckptname)
{
    char logname[512];
    int length = strlen(ckptname);
    strcpy(logname, ckptname);
    int i=0;
    for(i=length-1;i>=0;i--){
        if(logname[i]=='.'){
            strcpy(&logname[i], ".runlog");
            break;
        }
    }
    printf("running log file: %s\n", logname);
    if(access(logname, R_OK) == -1)
        logfile=open(logname, O_RDWR | O_CREAT);
    else
        logfile=open(logname, O_RDWR);
}

void init_start(uint64_t max_inst, uint64_t warmup_inst, uint64_t max_period, char *ckptname)
{ 
    printf("warmup: %d, maxinst: %d, period: %d\n", warmup_inst, max_inst, max_period);
    open_logfile(ckptname);
    

    Save_Basic_Regs();
    tempStackMem[32] = (uint64_t) &tempStackMem[2048]; //set sp
    tempStackMem[33] = (uint64_t) &tempStackMem[2560]; //set s0 = sp + 512*8
    printf("sample function addr: 0x%lx, sp: 0x%lx, s0: 0x%lx\n", &__sampleFucAddr, tempStackMem[32], tempStackMem[33]); 
    startcycle = read_csr_cycle();
    startinst = read_csr_instret();

    maxevent = max_inst-1000000, warmupinst = warmup_inst, eventsel=0, maxperiod = max_period;
    SetCounterLevel("0");
	RESET_COUNTER;
    SetPfcEnable(1);
    SetSampleBaseInfo(procTag, &__sampleFucAddr);
    SetSampleCtrlReg(maxevent, warmupinst, eventsel);
}


void sample_func()
{
    asm volatile("__sampleFucAddr: ");
    Save_ALLIntRegs();
    Load_Basic_Regs();

    GetExitPC(exitpc);
    SetTempReg(exitpc, 0);
    sampleHapTimes++;

    endcycle = read_csr_cycle();
    endinst = read_csr_instret();
    sprintf(str, "{\"type\": \"max_inst\", \"times\": %d, \"cycles\": %ld, \"inst\": %ld}\n", sampleHapTimes, endcycle - startcycle, endinst - startinst);
    sprintf(str, "sampleHapTimes: %d\n", sampleHapTimes);
    write(1, str, strlen(str));  
    write(logfile, str, strlen(str));  

    read_counters();
    startcycle = endcycle;
    startinst = endinst;

    if(sampleHapTimes >= maxperiod) {
        exit(0);
    }

    warmupinst = 0;
    maxevent = 0;
    RESET_COUNTER;
    SetPfcEnable(1);
    SetSampleBaseInfo(procTag, &__sampleFucAddr);
    SetSampleCtrlReg(maxevent, warmupinst, eventsel);
    Load_ALLIntRegs();
    JmpTempReg(0);
}



void read_counters()
{
	ReadCounter16(&hpcounters[0], 0);
	ReadCounter16(&hpcounters[16], 16);
	ReadCounter16(&hpcounters[32], 32);
	ReadCounter16(&hpcounters[48], 48);

	for(int n=0;n<64;n++){
		sprintf(str, "{\"type\": \"event %2d\", \"value\": %llu}\n", n, hpcounters[n]);
        write(1, str, strlen(str));
        write(logfile, str, strlen(str));  
    }
}

#endif 
