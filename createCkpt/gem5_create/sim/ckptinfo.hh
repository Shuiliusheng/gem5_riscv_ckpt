#ifndef __SIM_CKPTINFO_HH__
#define __SIM_CKPTINFO_HH__


#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <stdlib.h>
#include "debug/CreateCkpt.hh"
#include "base/trace.hh"
using namespace std;

//RISCV_Ckpt_Support: new file for storing the information of each ckpt

typedef struct{
  uint64_t addr;
  uint64_t size;
}CodeRange;

typedef struct{
  uint64_t addr;
  uint8_t data[8];
  uint8_t first;
  unsigned char size;
}LoadInfo;

typedef struct{
  uint64_t addr;
  uint8_t data[8];
}FirstLoadInfo;
// uint64_t size;
//firstload最终都被填充为8B，因此忽略大小


class SyscallInfo{
  public:
    string name;
    uint64_t pc;
    uint64_t num;
    uint64_t ret;
    bool hasret;
    uint64_t bufaddr;
    vector<uint8_t> data;
    vector<uint64_t> params;

    SyscallInfo() {
      pc = 0;
      num = 0;
      ret = 0;
      hasret = false;
      bufaddr = 0;
      data.clear();
      params.clear();
    }
};

#define AccessRange 8192
#define AccessRangeBits 13
#define RecordInstNum 1000
class CkptInfo{
  public:
    char filename[300];
    uint64_t startnum, simNums, length, exit_pc, pc, npc, warmup;
    uint64_t intregs[32], fpregs[32], instinfo[10], instinfo1[10];
    uint64_t textsize, memsize;
    vector<LoadInfo> loads;
    vector<FirstLoadInfo> firstloads;
    vector<CodeRange> memrange;
    vector<CodeRange> textrange;
    vector<SyscallInfo> sysinfos;
    map<uint64_t, uint8_t *> preAccess;
    set<uint64_t> textAccess;
    // exit_information
    map<uint32_t, uint32_t> instNumInfo;
    uint32_t instNumRes[RecordInstNum+1];
    uint64_t nowInstNum;

    CkptInfo(uint64_t startnum, uint64_t length, uint64_t warmup, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc, char filename[], uint64_t instinfo[]){
      this->startnum = startnum;
      for(int i=0;i<32;i++){
        this->intregs[i] = intregs[i];
        this->fpregs[i] = fpregs[i];
      }
      this->loads.clear();
      this->firstloads.clear();
      this->memrange.clear();
      this->textrange.clear();
      this->sysinfos.clear();
      this->preAccess.clear();
      this->textAccess.clear();
      this->instNumInfo.clear();
      this->length = length;
      this->pc = pc;
      this->npc = npc;
      this->warmup = warmup;
      nowInstNum = 0;

      if(strlen(filename) < 1) {
        strcpy(this->filename, "bench");
      }
      else {
        strcpy(this->filename, filename);
      }

      for(int i=0;i<10;i++){
        this->instinfo[i] = instinfo[i];
      }
    }
    ~CkptInfo(){
      this->loads.clear();
      this->preAccess.clear();
      this->textAccess.clear();
      this->firstloads.clear();
      this->memrange.clear();
      this->textrange.clear();
      this->sysinfos.clear();
      this->instNumInfo.clear();
    }

  public:
    void addAddrInfo(uint64_t addr, uint8_t size, uint8_t exist[]);

    static bool cmp(LoadInfo &info1, LoadInfo &info2) { return info1.addr < info2.addr; }
    void addload(uint64_t addr, uint8_t *data, unsigned char size);
    void addstore(uint64_t addr, unsigned char size);
    void addinst(uint64_t addr);
    uint32_t getinstNum(uint64_t addr);

    void add_sysenter(uint64_t pc, uint32_t sysnum, vector<uint64_t> &params);
    void add_sysret(uint64_t pc, string name, bool hasret, uint64_t ret);
    void add_sysexe(uint64_t pc, uint64_t ret, uint64_t bufaddr, uint32_t bufsize, uint8_t data[]);
    
    void combine_loads(vector<LoadInfo> &temploads);
    void getFistloads();
    uint64_t getRange(set<uint64_t> &addrs, vector<CodeRange> &ranges);
    uint64_t getRange(map<uint64_t, uint8_t *> &addrs, vector<CodeRange> &ranges);
    
    bool detectOver(uint64_t exit_place, uint64_t exit_pc, uint64_t instinfo[]);
    void showCkptInfo();
    void showSysInfo();

    void saveDetailInfo();
    void saveCkptInfo();
    void saveSysInfo(FILE *p);
    void saveSysInfo1(FILE *p);
};


void saveFLoads(vector<FirstLoadInfo> &firstloads, FILE *q);
void saveFLoads_comp(vector<FirstLoadInfo> &firstloads, FILE *q);

#endif