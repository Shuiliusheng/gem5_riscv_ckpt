#ifndef __SIM_CKPT_COLLECT_HH__
#define __SIM_CKPT_COLLECT_HH__


#include <string>
#include <vector>
#include <set>
#include "sim/ckptinfo.hh"
using namespace std;

//RISCV_Ckpt_Support: new file for configing checkpoint and running settings, and collecting running information


typedef struct{
    uint64_t start;
    uint64_t end;
    uint64_t warmup;
}CkptCtrl;

#define BrkPoint_For_Ckpt 0x1400000

class CkptSettings{
public:
    char benchname[300];
    uint64_t mmapend;
    uint64_t stack_base;
    uint64_t brk_point;

    vector<CkptCtrl> ctrls;

    CkptSettings(){
        strcpy(benchname, "");
        mmapend = 0;
        stack_base = 0;
        brk_point = 0;
        ctrls.clear();
    }

    ~CkptSettings() {
        ctrls.clear();
    }
};

void addCkpt(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc, uint64_t instinfo[]);
void ckpt_addload(uint64_t addr, uint8_t *data, unsigned char size);
void ckpt_addstore(uint64_t addr, unsigned char size);
void ckpt_addinst(uint64_t addr);

void ckpt_add_sysenter(uint64_t pc, uint32_t sysnum, vector<uint64_t> &params);
void ckpt_add_sysexe(uint64_t pc, uint64_t ret, uint64_t bufaddr, uint32_t bufsize, uint8_t data[]);
void ckpt_add_sysret(uint64_t pc, string name, bool hasret, uint64_t ret);

void ckpt_detectOver(uint64_t exit_place, uint64_t exit_pc, uint64_t instinfo[]);

void init_ckpt_settings(const char filename[]);
bool hasValidCkpt();
bool isCkptStart(uint64_t simNum, uint64_t &length);
void initCkptSysInfo(char *filename);
void ckpt_insert_syscall(uint32_t sysidx);
bool isCkptInst(uint64_t addr);

extern vector<CkptInfo *> pendingCkpts;
extern CkptSettings ckptsettings;
extern int ckptidx;
extern bool needCreateCkpt;
extern bool readCkptSetting;
extern uint64_t takeOverSysAddr;
extern uint64_t ckptstartnum;
extern bool strictLength;
extern uint64_t ckpt_textstart, ckpt_textend;
#endif