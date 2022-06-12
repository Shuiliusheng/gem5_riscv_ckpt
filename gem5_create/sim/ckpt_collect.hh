#ifndef __SIM_CKPT_COLLECT_HH__
#define __SIM_CKPT_COLLECT_HH__


#include <string>
#include <vector>
#include <set>
#include "sim/ckptinfo.hh"
using namespace std;

void addCkpt(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc);
void ckpt_addload(uint64_t addr, uint8_t *data, unsigned char size);
void ckpt_addstore(uint64_t addr, unsigned char size);
void ckpt_addinst(uint64_t addr);

void ckpt_add_sysenter(uint64_t pc, uint32_t sysnum, vector<uint64_t> &params);
void ckpt_add_sysexe(uint64_t pc, uint64_t ret, uint64_t bufaddr, uint32_t bufsize, uint8_t data[]);
void ckpt_add_sysret(uint64_t pc, string name, bool hasret, uint64_t ret);

void ckpt_detectOver(uint64_t exit_place, uint64_t exit_pc);

extern vector<CkptInfo *> pendingCkpts;
extern char benchname[300];
#endif