#include "sim/ckpt_collect.hh"

vector<CkptInfo *> pendingCkpts;
CkptSettings ckptsettings;
int ckptidx = 0;
bool needCreateCkpt = false;
bool readCkptSetting = false;

bool cmp(CkptCtrl &ctrl1, CkptCtrl &ctrl2) { 
  return ctrl1.start < ctrl2.start; 
}

void init_ckpt_settings(const char filename[])
{
  if(strlen(filename) < 1) return ;

  FILE *p=NULL;
  p=fopen(filename,"r");
  if(p==NULL) {
    printf("%s cannot be opened\n", filename);
    return ;
  }

  char str[100];
  uint64_t v1, v2, v3, v4;
  while(!feof(p)) {
    fgets(str, 100, p);
    if(strlen(str) < 1) break;
    str[strlen(str)-1] = '\0';

    string temp(str);
    uint64_t idx = temp.find(":");
    if(idx == temp.npos) {
      printf("%s", str);
      strcpy(str, "");
      continue;
    }
    if(temp.find("mmapend") != temp.npos) {
      sscanf(&str[idx+1], "%lld", &v1);
      printf("mmapend: 0x%lx\n", v1);
      ckptsettings.mmapend = v1;
    }
    else if(temp.find("stacktop") != temp.npos) {
      sscanf(&str[idx+1], "%lld", &v1);
      printf("stacktop: 0x%lx\n", v1);
      ckptsettings.stack_base = v1;
    }
    else if(temp.find("ckptprefix") != temp.npos) {
      idx = idx + 1;
      for(; idx<strlen(str) && str[idx] ==' '; idx++);
      printf("benchname: %s\n", &str[idx]);
      strcpy(ckptsettings.benchname,  &str[idx]);
    }
    else if(temp.find("ckptctrl") != temp.npos) {
      sscanf(&str[idx+1], "%lld %lld %lld %lld", &v1, &v2, &v3, &v4);
      CkptCtrl ctrl;
      for(int i=0; i<v4; i++) {
        ctrl.start = v1 + v2*i - v3;
        ctrl.end = ctrl.start + v2 + v3;
        ctrl.warmup = v3;
        ckptsettings.ctrls.push_back(ctrl);
      }
    }
    else if(temp.find("readckpt") != temp.npos) {
      idx = idx + 1;
      for(; idx<strlen(str) && str[idx] ==' '; idx++);
      printf("create ckpt with readckpt, ckptfile: %s\n", &str[idx]);
      initCkptSysInfo(&str[idx]);
      readCkptSetting = true;
    }
    else{
      printf("%s", str);
    }
    strcpy(str, "");
  }
  sort(ckptsettings.ctrls.begin(), ckptsettings.ctrls.end(), cmp);
  for(int i=0;i<ckptsettings.ctrls.size();i++){
    printf("ckpt info, start: %lld, end: %lld, warmup: %lld\n", ckptsettings.ctrls[i].start, ckptsettings.ctrls[i].end, ckptsettings.ctrls[i].warmup);
  }
  fclose(p);
}


void addCkpt(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc, uint64_t instinfo[]) 
{
  uint64_t warmup = ckptsettings.ctrls[ckptidx-1].warmup;
  pendingCkpts.push_back(new CkptInfo(startnum, length, warmup, intregs, fpregs, pc, npc, ckptsettings.benchname, instinfo));
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
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->add_sysenter(pc, sysnum, params);
  }
}

void ckpt_add_sysexe(uint64_t pc, uint64_t ret, uint64_t bufaddr, uint32_t bufsize, uint8_t data[]) 
{
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->add_sysexe(pc, ret, bufaddr, bufsize, data);
  }
}

void ckpt_add_sysret(uint64_t pc, string name, bool hasret, uint64_t ret)
{
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->add_sysret(pc, name, hasret, ret);
  }
}

void ckpt_detectOver(uint64_t exit_place, uint64_t exit_pc, uint64_t instinfo[]) 
{
  for (vector<CkptInfo *>::iterator it = pendingCkpts.begin(); it != pendingCkpts.end();) {
    if ((*it)->detectOver(exit_place, exit_pc, instinfo)) {
      printf("exit ckpt start with: %d\n", (*it)->startnum);
      delete *it;
      it = pendingCkpts.erase(it);
    } else {
      ++it;
    }
  }
}

bool hasValidCkpt() 
{
    if(pendingCkpts.size()!= 0) {
        return true;
    }
    else{
        return false;
    }
}

bool isCkptStart(uint64_t simNum, uint64_t &length)
{
    if(ckptidx >= ckptsettings.ctrls.size()) {
        return false;
    }

    if(simNum == ckptsettings.ctrls[ckptidx].start) {
        length = ckptsettings.ctrls[ckptidx].end - ckptsettings.ctrls[ckptidx].start;
        ckptidx++;
        return true;
    }
    else{
        return false;
    }
}

uint64_t syscall_info_addr = 0;
uint64_t totalcallnum = 0;
uint64_t infoaddr = 0;
uint32_t *sysidxs = NULL;

void initCkptSysInfo(char *filename)
{
  FILE *p = fopen(filename,"rb");
  if(p == NULL){
    printf("cannot open %s to read\n", filename);
    exit(1);
  }
  uint64_t numtext = 0, nummem = 0, numloads = 0, offset = 0;
  fread(&numtext, sizeof(uint64_t), 1, p);
  
  offset = 8 + numtext*16 + 5*8 + 64*8;
  fseek(p, offset, SEEK_SET);
  fread(&nummem, sizeof(uint64_t), 1, p);
  
  offset = ftell(p) + nummem*16;
  fseek(p, offset, SEEK_SET);
  fread(&numloads, sizeof(uint64_t), 1, p);
  offset = ftell(p) + numloads*16;
  
  fseek(p, 0, SEEK_END);
  uint64_t filesize = ftell(p) - offset;
  fseek(p, offset, SEEK_SET);

  uint8_t *data = (uint8_t *)malloc(filesize);
  fread((void *)data, filesize, 1, p);
  fclose(p);

  syscall_info_addr = (uint64_t)data;
  totalcallnum = *((uint64_t *)syscall_info_addr);
  infoaddr = syscall_info_addr + 8 + totalcallnum*4;
  sysidxs = (uint32_t *)(syscall_info_addr + 8);

  printf("%s: text: %d, mem: %d, load: %d, sysnum: %d\n", filename, numtext, nummem, numloads, totalcallnum);
}

typedef struct{
    uint64_t pc;
    uint64_t num;
    uint64_t p0;
    uint64_t ret;
    uint64_t data_offset;
}NewSyscallInfo;

void ckpt_insert_syscall(uint32_t sysidx) 
{
  if(sysidx > totalcallnum) {
    printf("syscall num %d is overflow\n", sysidx);
    exit(1);
  }

  if(sysidx == totalcallnum)
    return ;
  
  NewSyscallInfo *infos = (NewSyscallInfo *)(infoaddr + sysidxs[sysidx]*sizeof(NewSyscallInfo));
  uint64_t sysnum = infos->num >> 32;
  bool hasret = (infos->data_offset % 256) == 1;
  uint64_t bufsize = 0;
  uint64_t bufaddr = 0;
  uint64_t data_offset = infos->data_offset >> 8;
  uint8_t *data = NULL;
  string name("readckpt_syscall");
  vector<uint64_t> params;
  
  if (data_offset != 0xffffffff){
    bufaddr = *(uint64_t *)(syscall_info_addr + data_offset);
    data = (uint8_t *)(syscall_info_addr + data_offset + 8);
    bufsize = (infos->num<<32) >> 32;
  }
  params.push_back(infos->p0);
  params.push_back(infos->p0);
  params.push_back(infos->p0);
  
  for(int i=0;i<pendingCkpts.size();i++){
    pendingCkpts[i]->add_sysenter(infos->pc, sysnum, params);
    pendingCkpts[i]->add_sysexe(infos->pc, infos->ret, bufaddr, bufsize, data);
    pendingCkpts[i]->add_sysret(infos->pc, name, hasret, infos->ret);
  }
  params.clear();
}


