#include "sim/ckpt_collect.hh"

void CkptInfo::addload(uint64_t addr, uint8_t *data, unsigned char size) {
  uint8_t isFirst = 0;
  uint64_t ta = addr + size - 1;
  LoadInfo info;
  for(int i=size-1; i>=0; i--,ta--) {
    isFirst = isFirst << 1;
    if(this->preAccess.find(ta) == this->preAccess.end()) {
      isFirst = isFirst + 1;
      info.data[i] = data[i];
    }
    this->preAccess.insert(ta);
  }

  if(isFirst != 0) {
    info.first = isFirst;
    info.addr = addr;
    info.size = size;
    this->loads.push_back(info);
  }
}

void CkptInfo::addstore(uint64_t addr, unsigned char size) {
  for(int i=0; i<size; i++,addr++){
    this->preAccess.insert(addr);
  }
}

void CkptInfo::addinst(uint64_t addr) {
  for(int i=0; i<4; i++,addr++){
    this->textAccess.insert(addr);
  }
}

void CkptInfo::combine_loads(vector<LoadInfo> &temploads) {
  int size = temploads.size();
  if(size == 0) return ;

  sort(temploads.begin(), temploads.end(), this->cmp);
  long long len = 0;
  LoadInfo info;
  info.size = 8;
  info.addr = temploads[0].addr;
  *(uint64_t *)(info.data) = 0;
  info.data[0] = temploads[0].data[0];

  for(int i=1;i<size;i++){
    len = temploads[i].addr - info.addr;
    if(len < 8) {
      info.data[len] = temploads[i].data[0];
    }
    else{
      this->firstloads.push_back(info);
      info.addr = temploads[i].addr;
      *(uint64_t *)(info.data) = 0;
      info.data[0] = temploads[i].data[0];
    }
  }
  this->firstloads.push_back(info);
}

void CkptInfo::getFistloads() {
  vector<LoadInfo> temploads;
  uint8_t isFirst = 0;
  LoadInfo info;
  info.size = 1;
  for(int i=0;i<this->loads.size();i++){
    isFirst = this->loads[i].first;
    for(int j=0; j<this->loads[i].size; j++){
      if(isFirst%2 == 1){
        info.addr = this->loads[i].addr + j;
        info.data[0] = this->loads[i].data[j];
        temploads.push_back(info);
      }
      isFirst = isFirst >> 1;
    }
  }
  this->loads.clear();
  combine_loads(temploads);
}

void CkptInfo::getRange(set<uint64_t> &addrs, vector<CodeRange> &ranges) {
  set<uint64_t>::iterator iter = addrs.begin();
  uint64_t eaddr = 0;
  int64_t len = 0;
  CodeRange r;
  r.addr = ((*iter) >> 12) << 12;
  r.size = 4096;
  eaddr = r.addr + r.size;

  for(iter = addrs.begin() ; iter != addrs.end() ; ++iter) {
    len = *iter - eaddr;
    if(len < 0) continue;
    if(len < 4096) {
      r.size += 4096;
      eaddr = r.addr + r.size;
    }
    else{
      ranges.push_back(r);
      r.addr = ((*iter) >> 12) << 12;
      r.size = 4096;
      eaddr = r.addr + r.size;
    }
  }
  ranges.push_back(r);
}

bool CkptInfo::detectOver(uint64_t exit_place, uint64_t exit_pc) {
  bool isOver = false;
  if(exit_place >= this->startnum + this->length) {
    isOver = true;
    this->exit_pc = exit_pc;
    this->simNums = exit_place - this->startnum;
  }
  if(!isOver) {
    return false;
  }

  //add syscall buffer addr information to all addresses
  for(int i=0;i<sysinfos.size();i++){
    uint64_t addr = sysinfos[i].bufaddr;
    for(int j=0;j<sysinfos[i].data.size();i++, addr++){
      this->preAccess.insert(addr);
    }
  }

  this->getFistloads();
  this->getRange(this->textAccess, this->textrange);
  this->getRange(this->preAccess, this->memrange);
  saveCkptInfo();
  saveSysInfo();
  return isOver;
}

void CkptInfo::saveCkptInfo() {
  DPRINTF(ShowSyscall, "{\"type\": \"ckptinfo\", \"startnum\": \"%ld\", \"exitnum\": \"%ld\", \"length\": \"%ld\", \"pc\": \"0x%lx\", \"npc\": \"0x%lx\", \"exitpc\": \"0x%lx\"}\n", startnum, simNums, length, pc, npc, exit_pc);

  char str[3000];
  sprintf(str, "{\"type\": \"int_regs\", \"data\": [ ");
  for(int i=0;i<31;i++){
      sprintf(str, "%s\"0x%lx\", ", str, intregs[i]);
  }
  DPRINTF(ShowSyscall, "%s\"0x%lx\" ]}\n", str, intregs[31]);

  sprintf(str, "{\"type\": \"fp_regs\", \"data\": [ ");
  for(int i=0;i<31;i++){
      sprintf(str, "%s\"0x%lx\", ", str, fpregs[i]);
  }
  DPRINTF(ShowSyscall, "%s\"0x%lx\" ]}\n", str, fpregs[31]);

  sprintf(str, "{\"type\": \"textRange\", \"addr\": [ ");
  int size = textrange.size();
  int i=0;
  for(i=0;i<size-1;i++){
      sprintf(str, "%s\"0x%lx\", ", str, textrange[i].addr);
  }
  sprintf(str, "%s\"0x%lx\" ], \"size\": [ ", str, textrange[size-1].addr);
  for(i=0;i<size-1;i++){
      sprintf(str, "%s\"0x%x\", ", str, textrange[i].size);
  }
  DPRINTF(ShowSyscall, "%s\"0x%x\" ] }\n", str, textrange[size-1].size);

  sprintf(str, "{\"type\": \"memRange\", \"addr\": [ ");
  size = memrange.size();
  for(i=0;i<size-1;i++){
      sprintf(str, "%s\"0x%lx\", ", str, memrange[i].addr);
  }
  sprintf(str, "%s\"0x%lx\" ], \"size\": [ ", str, memrange[size-1].addr);
  for(i=0;i<size-1;i++){
      sprintf(str, "%s\"0x%x\", ", str, memrange[i].size);
  }
  DPRINTF(ShowSyscall, "%s\"0x%x\" ] }\n", str, memrange[size-1].size);

  for(int i=0;i<firstloads.size();i++) {
    DPRINTF(ShowSyscall, "{\"type\": \"fld\", \"a\": \"0x%lx\", \"s\": \"0x%x\", \"d\": \"0x%lx\"}\n", firstloads[i].addr, firstloads[i].size, *((uint64_t *)firstloads[i].data));
  }

  printf("ckptinfo, startnum: %ld, exitnum: %ld, length: %ld, pc: 0x%lx, npc: 0x%lx, exitpc: 0x%lx\n", startnum, simNums, length, pc, npc, exit_pc);
  printf("text range num: %ld, mem range num: %ld, first load num: %ld\n", textrange.size(), memrange.size(), firstloads.size());

  preAccess.clear();
  textAccess.clear();
  firstloads.clear();
  memrange.clear();
  textrange.clear();
}


void CkptInfo::saveSysInfo() 
{
  for(int i=0;i<sysinfos.size(); i++){
    DPRINTF(ShowSyscall, "{\"type\": \"syscall\", \"pc\": \"0x%lx\", \"sysnum\": \"0x%x\", \"sysname\": \"%s\", \"hasret\": \"%d\", \"ret\": \"0x%lx\", \"params\":[\"0x%lx\", \"0x%lx\"]}\n", sysinfos[i].pc, sysinfos[i].num, sysinfos[i].name.c_str(), sysinfos[i].hasret, sysinfos[i].ret, sysinfos[i].params[0], sysinfos[i].params[1]);

    if(sysinfos[i].data.size() > 0) {
      char *str = (char *)malloc(1000 + sysinfos[i].data.size()*8);
      sprintf(str, "{\"bufaddr\": \"0x%lx\", \"size\": \"0x%lx\", \"data\": [ ", sysinfos[i].bufaddr, sysinfos[i].data.size());
      int size = sysinfos[i].data.size()-1;
      for(int j=0;j<size;j++){
        sprintf(str, "%s\"0x%x\",", str, sysinfos[i].data[j]);
      }
      DPRINTF(ShowSyscall, "%s\"0x%x\" ]}\n", str, sysinfos[i].data[size]);
      free(str);
    }
  }

  sysinfos.clear();
}


//////////////////////////////////////////////////////////////////////////////////////////
vector<CkptInfo *> pendingCkpts;

void addCkpt(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc) {
  pendingCkpts.push_back(new CkptInfo(startnum, length, intregs, fpregs, pc, npc));
}

void ckpt_addload(uint64_t addr, uint8_t *data, unsigned char size) {
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->addload(addr, data, size);
  }
} 

void ckpt_addstore(uint64_t addr, unsigned char size) {
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->addstore(addr, size);
  }
} 

void ckpt_addinst(uint64_t addr) {
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->addinst(addr);
  }
} 

void ckpt_detectOver(uint64_t exit_place, uint64_t exit_pc) {
  for (vector<CkptInfo *>::iterator it = pendingCkpts.begin(); it != pendingCkpts.end();) {
    if ((*it)->detectOver(exit_place, exit_pc)) {
      delete *it;
      it = pendingCkpts.erase(it);
    } else {
      ++it;
    }
  }
}


/************************************************************************************/


void CkptInfo::add_sysenter(uint64_t pc, uint32_t sysnum, vector<uint64_t> &params) 
{
  SyscallInfo info;
  info.pc = pc;
  info.num = sysnum;
  info.params.assign(params.begin(),params.end());
  info.data.clear();
  this->sysinfos.push_back(info);
}

void CkptInfo::add_sysret(uint64_t pc, string name, bool hasret, uint64_t ret)
{
  int idx = sysinfos.size() - 1;
  if(idx < 0) return ;
  if(sysinfos[idx].pc != pc) {
    printf("add_sysret, sysinfo pc is not same, 0x%lx, 0x%lx\n", sysinfos[idx].pc, pc);
    return ;
  }

  sysinfos[idx].ret = ret;
  sysinfos[idx].hasret = hasret;
  sysinfos[idx].name = name;
}

void CkptInfo::add_sysexe(uint64_t pc, uint64_t ret, uint64_t bufaddr, uint32_t bufsize, uint8_t data[]) 
{
  int idx = sysinfos.size() - 1;
  if(idx < 0) return ;
  if(sysinfos[idx].pc != pc) {
    printf("add_sysexe, sysinfo pc is not same, 0x%lx, 0x%lx\n", sysinfos[idx].pc, pc);
    return ;
  }

  sysinfos[idx].ret = ret;
  sysinfos[idx].bufaddr = bufaddr;
  for(int i=0;i<bufsize;i++){
    sysinfos[idx].data.push_back(data[i]);
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




