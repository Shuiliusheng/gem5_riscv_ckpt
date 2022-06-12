#include "sim/ckpt_collect.hh"

vector<CkptInfo *> pendingCkpts;
char benchname[300] = "";


void addCkpt(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc) 
{
  pendingCkpts.push_back(new CkptInfo(startnum, length, intregs, fpregs, pc, npc, benchname));
}

void ckpt_addload(uint64_t addr, uint8_t *data, unsigned char size) 
{
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->addload(addr, data, size);
  }
} 

void ckpt_addstore(uint64_t addr, unsigned char size) 
{
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->addstore(addr, size);
  }
} 

void ckpt_addinst(uint64_t addr) 
{
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->addinst(addr);
  }
} 

void ckpt_add_sysenter(uint64_t pc, uint32_t sysnum, vector<uint64_t> &params) 
{
  if(pendingCkpts.size() > 0) {
    pendingCkpts[pendingCkpts.size()-1]->add_sysenter(pc, sysnum, params);
  }
}

void ckpt_add_sysexe(uint64_t pc, uint64_t ret, uint64_t bufaddr, uint32_t bufsize, uint8_t data[]) 
{
  if(pendingCkpts.size() > 0) {
    pendingCkpts[pendingCkpts.size()-1]->add_sysexe(pc, ret, bufaddr, bufsize, data);
  }
}

void ckpt_add_sysret(uint64_t pc, string name, bool hasret, uint64_t ret)
{
  if(pendingCkpts.size() > 0) {
    pendingCkpts[pendingCkpts.size()-1]->add_sysret(pc, name, hasret, ret);
  }
}

void ckpt_detectOver(uint64_t exit_place, uint64_t exit_pc) 
{
  for (vector<CkptInfo *>::iterator it = pendingCkpts.begin(); it != pendingCkpts.end();) {
    if ((*it)->detectOver(exit_place, exit_pc)) {
      delete *it;
      it = pendingCkpts.erase(it);
    } else {
      ++it;
    }
  }
}





