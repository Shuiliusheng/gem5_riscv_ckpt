#ifndef __SIM_CKPT_COLLECT_HH__
#define __SIM_CKPT_COLLECT_HH__


#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <stdlib.h>
#include "debug/ShowSyscall.hh"
#include "base/trace.hh"
using namespace std;

#include <set>
#include <vector>
using namespace std;

typedef struct{
  uint64_t addr;
  unsigned int size;
}CodeRange;

typedef struct{
  uint64_t addr;
  uint8_t data[8];
  uint8_t first;
  unsigned char size;
}LoadInfo;

class SyscallInfo{
  public:
    string name;
    uint64_t pc;
    uint32_t num;
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

class CkptInfo{
  public:
    uint64_t startnum, simNums, length, exit_pc, pc, npc;
    uint64_t intregs[32], fpregs[32];
    vector<LoadInfo> loads;
    vector<LoadInfo> firstloads;
    vector<CodeRange> memrange;
    vector<CodeRange> textrange;
    vector<SyscallInfo> sysinfos;
    set<uint64_t> preAccess;
    set<uint64_t> textAccess;

    CkptInfo(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc){
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
      this->length = length;
      this->pc = pc;
      this->npc = npc;
      DPRINTF(ShowSyscall, "{\"type\": \"startckpt\", \"startnum\": \"%ld\"}\n", startnum);

    }
    ~CkptInfo(){
      this->loads.clear();
      this->preAccess.clear();
      this->textAccess.clear();
      this->firstloads.clear();
      this->memrange.clear();
      this->textrange.clear();
      this->sysinfos.clear();
    }

  public:
    void addload(uint64_t addr, uint8_t *data, unsigned char size);
    void addstore(uint64_t addr, unsigned char size);
    void addinst(uint64_t addr);
    void combine_loads(vector<LoadInfo> &temploads);
    void getFistloads();
    void getRange(set<uint64_t> &addrs, vector<CodeRange> &ranges);
    bool detectOver(uint64_t exit_place, uint64_t exit_pc);
    void saveCkptInfo();
    void saveSysInfo();
    static bool cmp(LoadInfo &info1, LoadInfo &info2) {
      return info1.addr < info2.addr;
    }

    void add_sysenter(uint64_t pc, uint32_t sysnum, vector<uint64_t> &params);
    void add_sysret(uint64_t pc, string name, bool hasret, uint64_t ret);
    void add_sysexe(uint64_t pc, uint64_t ret, uint64_t bufaddr, uint32_t bufsize, uint8_t data[]);
};




extern vector<CkptInfo *> pendingCkpts;

void addCkpt(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc);
void ckpt_addload(uint64_t addr, uint8_t *data, unsigned char size);
void ckpt_addstore(uint64_t addr, unsigned char size);
void ckpt_addinst(uint64_t addr);
void ckpt_detectOver(uint64_t exit_place, uint64_t exit_pc);

void ckpt_add_sysenter(uint64_t pc, uint32_t sysnum, vector<uint64_t> &params);

void ckpt_add_sysexe(uint64_t pc, uint64_t ret, uint64_t bufaddr, uint32_t bufsize, uint8_t data[]);

void ckpt_add_sysret(uint64_t pc, string name, bool hasret, uint64_t ret);

#endif