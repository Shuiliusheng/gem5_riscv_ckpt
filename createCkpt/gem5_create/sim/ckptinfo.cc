#include "sim/ckptinfo.hh"
#include "sim/ckpt_collect.hh"
#include <malloc.h>

void CkptInfo::addAddrInfo(uint64_t addr, uint8_t size, uint8_t exist[])
{
  uint64_t base = (addr >> AccessRangeBits) << AccessRangeBits;
  uint8_t len1 = size, len2 = 0;
  uint64_t addr1 = base + AccessRange;
  if(addr + size -1 >= addr1) {
    len2 = addr + size - addr1; 
    len1 = size - len2;
  }

  map<uint64_t, uint8_t *>::iterator it = preAccess.find(base);
  if(it == preAccess.end()) {
    uint8_t *data = (uint8_t *)malloc(AccessRange);
    memset(data, 0, AccessRange);
    int idx = addr - base;
    for(int i=0;i<len1;i++,idx++) {
      exist[i] = 0;
      data[idx] = 1;
    }
    preAccess[base] = data;
  }
  else {
    uint8_t *data = it->second;
    int idx = addr - base;
    for(int i=0;i<len1;i++,idx++) {
      exist[i] = data[idx];
      data[idx] = 1;
    }
  }

  if(len2!=0) {
    it = preAccess.find(addr1);
    if(it == preAccess.end()) {
      uint8_t *data = (uint8_t *)malloc(AccessRange);
      memset(data, 0, AccessRange);
      for(int i=0;i<len2;i++) {
        exist[i+len1] = 0;
        data[i] = 1;
      }
      preAccess[addr1] = data;
    }
    else {
      uint8_t *data = it->second;
      for(int i=0;i<len1;i++) {
        exist[i+len1] = data[i];
        data[i] = 1;
      }
    }
  }
}

void CkptInfo::addload(uint64_t addr, uint8_t *data, unsigned char size) 
{
  uint8_t isFirst = 0;
  LoadInfo info;
  uint8_t exist[8];
  addAddrInfo(addr, size, exist);
  for(int i=size-1; i>=0; i--) {
    isFirst = isFirst << 1;
    if(exist[i]==0) {
      isFirst = isFirst + 1;
      info.data[i] = data[i];
    }
  }

  if(isFirst != 0) {
    info.first = isFirst;
    info.addr = addr;
    info.size = size;
    this->loads.push_back(info);
  }
}

void CkptInfo::addstore(uint64_t addr, unsigned char size) 
{
  uint8_t exist[8];
  addAddrInfo(addr, size, exist);
}

void CkptInfo::addinst(uint64_t addr) 
{
  uint64_t t1 = (addr >> 12) << 12;
  this->textAccess.insert(addr);
  if(t1+4096 < addr+4)
    this->textAccess.insert(addr+4096);

  // exit_information
  // nowInstNum++;
  // uint32_t temp = 1;
  // if(nowInstNum >= warmup) {
  //   map<uint32_t, uint32_t>::iterator it = instNumInfo.find((uint32_t)addr);
  
  //   if(it == instNumInfo.end()) {
  //     instNumInfo[(uint32_t)addr] = 1;
  //   }
  //   else {
  //     temp = instNumInfo[(uint32_t)addr] + 1;
  //     instNumInfo[(uint32_t)addr] = temp;
  //   }
  // }

  // if(nowInstNum >= length && nowInstNum < (length+RecordInstNum)) {
  //   instNumRes[nowInstNum-length] = temp;
  // }
}

uint32_t CkptInfo::getinstNum(uint64_t addr) 
{
  map<uint32_t, uint32_t>::iterator it = instNumInfo.find((uint32_t)addr);
  if(it == instNumInfo.end()) {
    return 1;
  }
  else {
    return instNumInfo[(uint32_t)addr];
  }
}



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


void CkptInfo::combine_loads(vector<LoadInfo> &temploads) 
{
  int size = temploads.size();
  if(size == 0) return ;

  sort(temploads.begin(), temploads.end(), this->cmp);
  long long len = 0;
  FirstLoadInfo info;
  // info.size = 8;
  info.addr = temploads[0].addr;
  *(uint64_t *)(info.data) = 0;
  info.data[0] = temploads[0].data[0];

  for(int i=1;i<size;i++){
    len = temploads[i].addr - info.addr;
    if(len < 8) {
      info.data[len] = temploads[i].data[0];
    }
    else{
      if(*(uint64_t *)(info.data) != 0)
        this->firstloads.push_back(info);
      info.addr = temploads[i].addr;
      *(uint64_t *)(info.data) = 0;
      info.data[0] = temploads[i].data[0];
    }
  }
  this->firstloads.push_back(info);
}

void CkptInfo::getFistloads() 
{
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
  vector<LoadInfo>().swap(this->loads);

  combine_loads(temploads);
  temploads.clear();
  vector<LoadInfo>().swap(temploads);
}

uint64_t CkptInfo::getRange(set<uint64_t> &addrs, vector<CodeRange> &ranges) 
{
  set<uint64_t>::iterator iter = addrs.begin();
  uint64_t eaddr = 0;
  int64_t len = 0;
  uint64_t totalsize = 0;
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
      totalsize += r.size;
      r.addr = ((*iter) >> 12) << 12;
      r.size = 4096;
      eaddr = r.addr + r.size;
    }
  }
  ranges.push_back(r);
  totalsize += r.size;
  return totalsize;
}

uint64_t CkptInfo::getRange(map<uint64_t, uint8_t *> &addrs, vector<CodeRange> &ranges) 
{
  map<uint64_t, uint8_t *>::iterator iter = addrs.begin();
  uint64_t eaddr = 0;
  int64_t len = 0;
  uint64_t totalsize = 0;
  CodeRange r;
  r.addr = ((iter->first) >> 12) << 12;
  r.size = AccessRange;
  eaddr = r.addr + r.size;

  for(iter = addrs.begin() ; iter != addrs.end() ; ++iter) {
    free(iter->second);
    len = iter->first - eaddr;
    if(len < 0) continue;
    if(len < AccessRange) {
      r.size += AccessRange;
      eaddr = r.addr + r.size;
    }
    else{
      ranges.push_back(r);
      totalsize += r.size;
      r.addr = ((iter->first) >> 12) << 12;
      r.size = AccessRange;
      eaddr = r.addr + r.size;
    }
  }
  ranges.push_back(r);
  totalsize += r.size;
  return totalsize;
}

bool CkptInfo::detectOver(uint64_t exit_place, uint64_t exit_pc, uint64_t instinfo[]) 
{
  bool isOver = false;
  if(exit_place >= this->startnum + this->length) {
    isOver = true;
    this->exit_pc = exit_pc;
    this->simNums = exit_place - this->startnum;
  }
  if(!isOver) {
    return false;
  }
  //update inst infor
  for(int i=0;i<10;i++){
    this->instinfo[i] = instinfo[i] - this->instinfo[i];
  }

  //add syscall buffer addr information to all addresses
  uint8_t exist[8];
  for(int i=0;i<sysinfos.size();i++){
    if(sysinfos[i].data.size()<1)
      continue;
    uint64_t addr = sysinfos[i].bufaddr;
    uint64_t base = (addr >> 12) << 12;
    addr = addr + sysinfos[i].data.size() + 4096;
    for(;base <= addr; base+=4096){
      addAddrInfo(base, 1, exist);
    }
  }

  this->getFistloads();
  for(int i=0;i<firstloads.size();i++){
    uint64_t t1 = (firstloads[i].addr >> 12) << 12;
    addAddrInfo(t1, 1, exist);
    if(t1+4096 < firstloads[i].addr+8){
      addAddrInfo(t1+4096, 1, exist);
    }
  }
  printf("preAccess size: %llu\n", preAccess.size());
  printf("textAccess size: %llu\n", textAccess.size());

  this->textsize = this->getRange(this->textAccess, this->textrange);
  this->memsize = this->getRange(this->preAccess, this->memrange);

  int len = textrange.size();
  int textend = textrange[len-1].addr + textrange[len-1].size;
  if(memrange.size() > 0 && memrange[0].addr < textend) {
    printf("memrange and textrange are overlap: 0x%lx, 0x%lx\n", textend, memrange[0].addr);
    vector<CodeRange>::iterator iter;
    for (iter = memrange.begin(); iter != memrange.end();){
      if (iter->addr < textend) {
        uint64_t temp = iter->addr + iter->size;
        if(temp > textend) {
          iter->addr = textend;
          iter->size = temp - textend;
          iter++;
        }
        else {
          iter = memrange.erase(iter);
        }
      }
      else {
        break;
      }
    }
  }

  uint64_t s1=preAccess.size()*AccessRange/1024;
  uint64_t s2=textAccess.size()*64/1024;
  uint64_t s3=firstloads.size()*128/1024;
  uint64_t s4=(memrange.size() + textrange.size())*128/1024;
  uint64_t totalsize = s1+s2+s3+s4;
  printf("preAccess memsize: %d KB\n", s1);
  printf("textAccess memsize: %d KB\n", s2);
  printf("firstloads memsize: %d KB\n", s3);
  printf("range memsize: %d KB\n", s4);
  printf("totalsize: %d MB\n", totalsize/1024);
  
  saveDetailInfo();
  saveCkptInfo();

  preAccess.clear();
  textAccess.clear();
  firstloads.clear();
  memrange.clear();
  textrange.clear();
  sysinfos.clear();
  instNumInfo.clear();

  map<uint64_t, uint8_t *>().swap(preAccess);
  map<uint32_t, uint32_t>().swap(instNumInfo);
  set<uint64_t>().swap(textAccess);
  vector<FirstLoadInfo>().swap(firstloads);
  vector<CodeRange>().swap(memrange);
  vector<CodeRange>().swap(textrange);
  vector<SyscallInfo>().swap(sysinfos);
  malloc_trim(0);

  return isOver;
}

void CkptInfo::showCkptInfo() 
{
  
}


void CkptInfo::showSysInfo() 
{
  
}


void CkptInfo::saveDetailInfo()
{
  char dstname[300];
  if(readCkptSetting)
    sprintf(dstname, "%s_ninfor_%ld_len_%d_warmup_%d.txt", this->filename, ckptstartnum + this->startnum + this->warmup, length, warmup);
  else
    sprintf(dstname, "%s_infor_%ld_len_%d_warmup_%d.txt", this->filename, this->startnum + this->warmup, length, warmup);

  FILE *p=NULL;
  p = fopen(dstname, "w");
  if(p == NULL){
    printf("cannot open %s for write\n", dstname);
  }
  
  fprintf(p, "ckptinfo, startnum: %lld, exitnum: %lld, length: %lld\n, warmup: %lld, pc: 0x%lx, npc: 0x%lx, exitpc: 0x%lx\n", startnum, simNums, length-warmup, warmup, pc, npc, exit_pc);
  fprintf(p, "text range num: %lld, mem range num: %lld, first load num: %lld, syscallnum: %lld\n\n", textrange.size(), memrange.size(), firstloads.size(), sysinfos.size());
  
  fprintf(p, "\n-- integer register value: --\n");
  for(int i=0;i<32;i++){
      fprintf(p, "reg %d: 0x%lx\n", i, intregs[i]);
  }

  fprintf(p, "\n-- float register value: --\n");
  for(int i=0;i<32;i++){
      fprintf(p, "reg %d: 0x%lx\n", i, fpregs[i]);
  }

  fprintf(p, "\n-- text range information: %lld KB --\n", textsize >> 10);
  for(int i=0;i<textrange.size();i++){
      fprintf(p, "text range %d: 0x%lx, %lld KB\n", i, textrange[i].addr, textrange[i].size >> 10);
  }

  fprintf(p, "\n-- mem range information: %lld KB --\n", memsize >> 10);
  for(int i=0;i<memrange.size();i++){
      fprintf(p, "mem range %d: 0x%lx, %lld KB\n", i, memrange[i].addr, memrange[i].size >> 10);
  }  

  fprintf(p, "\n--running instruction information --\n");
  fprintf(p, "isLoad: %lld, isStore: %lld, isAtomic: %lld\n", instinfo[0], instinfo[1], instinfo[2]); 
  fprintf(p, "isControl: %lld, isCall: %lld, isReturn: %lld\n", instinfo[3], instinfo[4], instinfo[5]); 
  fprintf(p, "isCondCtrl: %lld, isUncondCtrl: %lld, isIndirectCtrl: %lld\n", instinfo[6], instinfo[7], instinfo[8]); 

  // exit_information
  // fprintf(p, "\n-- last exit inst running number --\n");
  // uint64_t value = 0;
  // for(int i=0;i<RecordInstNum;i++){
  //   fprintf(p,"lastinst, %d, %d\n", i, instNumRes[i]);
  //   value += instNumRes[i];
  // }
  // fprintf(p,"average_lastinst, %d\n", value/RecordInstNum);

  fclose(p);
}

void CkptInfo::saveCkptInfo()
{
  char dstname[300];
  if(readCkptSetting)
    sprintf(dstname, "%s_nckpt_%ld_len_%d_warmup_%d.info", this->filename, ckptstartnum + this->startnum + this->warmup, length, warmup);
  else
    sprintf(dstname, "%s_ckpt_%ld_len_%d_warmup_%d.info", this->filename, this->startnum + this->warmup, length, warmup);

  FILE *p=NULL;
  p = fopen(dstname, "wb");
  if(p == NULL){
    printf("cannot open %s for write\n", dstname);
  }
  //text range 
  uint64_t temp = textrange.size();
  fwrite(&temp, sizeof(uint64_t), 1, p);
  fwrite(&textrange[0], sizeof(CodeRange), temp, p);

  //start information
  uint64_t mem[5];
  mem[0] = startnum;
  if(readCkptSetting)
    mem[0] = ckptstartnum + this->startnum;
  mem[1] = simNums + (warmup << 32);
  mem[2] = exit_pc;
  mem[3] = 2 + ((length-warmup) << 2);
  mem[4] = npc;
  fwrite(&mem[0], sizeof(uint64_t), 5, p);
  fwrite(&intregs[0], sizeof(uint64_t), 32, p);
  fwrite(&fpregs[0], sizeof(uint64_t), 32, p);

  //mem information
  temp = memrange.size();
  fwrite(&temp, sizeof(uint64_t), 1, p);
  fwrite(&memrange[0], sizeof(CodeRange), temp, p);

  //first load information
  temp = firstloads.size();
  fwrite(&temp, sizeof(uint64_t), 1, p);
  fwrite(&firstloads[0], sizeof(FirstLoadInfo), temp, p);

  //save syscall information 
  saveSysInfo(p);
  fclose(p);
}


bool isSameSyscall(SyscallInfo *info1, SyscallInfo *info2)
{
    if(info1->pc != info2->pc) 
      return false;
    if(info1->num != info2->num) 
      return false;
    if(info1->hasret != info2->hasret) 
      return false;
    if(info1->ret != info2->ret) 
      return false;
    if(info1->params[0] != info2->params[0]) 
      return false;
    if(info1->data.size() == 0)
      return true;
      
    if(info1->data.size() != info2->data.size()) 
      return false;
    if(info1->bufaddr != info2->bufaddr)
      return false;
    for(int i=0;i<info1->data.size();i++){
      if(info1->data[i]!=info2->data[i])
        return false;
    }

    return true;
}

uint32_t dectect_Sysinfos(vector<SyscallInfo> &sysinfos, uint32_t idx, set<uint64_t> &prepc, set<uint32_t> &prenum, vector<uint32_t> &newinfos)
{
  uint32_t res=0;
  if(prepc.find(sysinfos[idx].pc) == prepc.end() || prenum.find(sysinfos[idx].num) == prenum.end()) {
    newinfos.push_back(idx);
    res = newinfos.size()-1;
  }
  else {
    int c = 0;
    for(c = 0; c < newinfos.size(); c ++) {
      if(isSameSyscall(&sysinfos[idx], &sysinfos[newinfos[c]]))
        break;
    }
    if(c == newinfos.size()) {
      newinfos.push_back(idx);
      res = newinfos.size()-1;
    }
    else {
      res = c;
    }
  }
  prepc.insert(sysinfos[idx].pc);
  prenum.insert(sysinfos[idx].num);
  return res;
}

void CkptInfo::saveSysInfo(FILE *p)
{
  vector<uint32_t> sys_idxs;
  vector<uint32_t> newsysinfos, newsysinfos1; //no data
  set<uint64_t> prepc, prepc1;
  set<uint32_t> prenum, prenum1;
  uint32_t num1 = 0;
  for(int i=0;i<sysinfos.size();i++) {
    if(sysinfos[i].data.size() != 0){
      sys_idxs.push_back(dectect_Sysinfos(sysinfos, i, prepc1, prenum1, newsysinfos1));
      num1 ++;
    }
    else{
      sys_idxs.push_back(dectect_Sysinfos(sysinfos, i, prepc, prenum, newsysinfos));
    }
  }

  int size1 = newsysinfos.size();
  for(int i=0;i<sysinfos.size();i++) {
    if(sysinfos[i].data.size()!= 0) {
      sys_idxs[i] += size1;
    }
  }

  // printf("before syscall num: (%d %d), after num: (%d %d)\n", num1, sysinfos.size()-num1, newsysinfos1.size(), size1);

  uint64_t idx_size = 4*sysinfos.size();
  uint64_t info_size = 5*8*(newsysinfos.size()+newsysinfos1.size());
  uint64_t data_addr = idx_size + info_size + 8; 
  uint64_t invalid_addr = 0xffffffff; 
  uint64_t temp = sysinfos.size();
  fwrite(&temp, sizeof(uint64_t), 1, p);
  fwrite(&sys_idxs[0], sizeof(uint32_t), temp, p);

  uint64_t mem[10];
  for(int i=0; i<newsysinfos.size(); i++) {
    int idx = newsysinfos[i];
    mem[0] = sysinfos[idx].pc;
    mem[1] = (sysinfos[idx].num << 32) + sysinfos[idx].data.size();
    mem[2] = sysinfos[idx].params[0];
    mem[3] = sysinfos[idx].ret;
    mem[4] = (sysinfos[idx].hasret) ? (invalid_addr<<8) + 1 : (invalid_addr<<8) + 0;
    fwrite(&mem[0], sizeof(uint64_t), 5, p);
  }

  for(int i=0; i<newsysinfos1.size(); i++) {
    int idx = newsysinfos1[i];
    mem[0] = sysinfos[idx].pc;
    mem[1] = (sysinfos[idx].num << 32) + sysinfos[idx].data.size();
    mem[2] = sysinfos[idx].params[0];
    mem[3] = sysinfos[idx].ret;
    mem[4] = (sysinfos[idx].hasret) ? (data_addr<<8) + 1 : (data_addr<<8) + 0;
    fwrite(&mem[0], sizeof(uint64_t), 5, p);
    data_addr = data_addr + sysinfos[idx].data.size() + sizeof(uint64_t);  //add the bufaddr information size
  }

  for(int i=0; i<newsysinfos1.size(); i++) {
    int idx = newsysinfos1[i];
    fwrite(&sysinfos[idx].bufaddr, sizeof(uint64_t), 1, p);
    fwrite(&sysinfos[idx].data[0], 1, sysinfos[idx].data.size(), p);
    vector<uint8_t>().swap(sysinfos[idx].data);
  }

  sys_idxs.clear();
  newsysinfos.clear(), newsysinfos1.clear(); //no data
  prepc.clear(), prepc1.clear();
  prenum.clear(), prenum1.clear();

  vector<uint32_t>().swap(sys_idxs);
  vector<uint32_t>().swap(newsysinfos);
  vector<uint32_t>().swap(newsysinfos1);
  set<uint64_t>().swap(prepc);
  set<uint64_t>().swap(prepc1);
  set<uint32_t>().swap(prenum);
  set<uint32_t>().swap(prenum1);
}


void CkptInfo::saveSysInfo1(FILE *p)
{
  uint64_t data_addr = 10*8*sysinfos.size() + 8; 
  uint64_t invalid_addr = 0xffffffff; 
  uint64_t temp = sysinfos.size();

  fwrite(&temp, sizeof(uint64_t), 1, p);
  uint64_t mem[10];
  for(int i=0; i<temp; i++) {
    mem[0] = sysinfos[i].pc;
    mem[1] = sysinfos[i].num;
    mem[2] = sysinfos[i].params[0];
    mem[3] = sysinfos[i].params[1];
    mem[4] = sysinfos[i].params[2];
    mem[5] = sysinfos[i].hasret;
    mem[6] = sysinfos[i].ret;
    mem[7] = sysinfos[i].bufaddr;
    mem[8] = data_addr;
    mem[9] = sysinfos[i].data.size();
    if(sysinfos[i].data.size() == 0) {
      mem[8] = invalid_addr;
    }
    fwrite(&mem[0], sizeof(uint64_t), 10, p);
    data_addr = data_addr + sysinfos[i].data.size();
  }

  for(int i=0; i<temp; i++) {
    fwrite(&sysinfos[i].data[0], 1, sysinfos[i].data.size(), p);
    vector<uint8_t>().swap(sysinfos[i].data);
  }
}