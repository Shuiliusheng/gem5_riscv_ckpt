#include "sim/trace_collect.hh"

namespace gem5
{
typedef struct{
    uint64_t start;
    uint64_t end;
}TraceCtrl;

char logname[512];
FILE *logfp = NULL;
bool traceOpen = true;
uint64_t maxpc = 0x9F0000000, minpc = 0;
uint64_t ckptstart = 0;
uint64_t exeInstNum = 0;
MemAddrinfo memInfo;
vector<TraceCtrl> tracectrls;
uint32_t ctrlidx = 0;


bool cmp(TraceCtrl &ctrl1, TraceCtrl &ctrl2) { 
  return ctrl1.start < ctrl2.start; 
}

void init_trace_settings(const char filename[])
{
  traceOpen = true;
  
  if(strlen(filename) < 1) { traceOpen = false; return ; }

  FILE *p=NULL;
  p=fopen(filename,"r");
  if(p==NULL) {
    printf("%s cannot be opened\n", filename);
    traceOpen = false;
    return ;
  }

  char str[256];
  uint64_t v1, v2, v3;
  while(!feof(p)) {
    fgets(str, 300, p);
    if(strlen(str) < 1) break;
    str[strlen(str)-1] = '\0';

    string temp(str);
    uint64_t idx = temp.find(":");
    if(idx == temp.npos) {
      printf("%s", str);
      strcpy(str, "");
      continue;
    }
    if(temp.find("maxpc") != temp.npos) {
      sscanf(&str[idx+1], "%llx", &v1);
      maxpc = v1;
    }
    else if(temp.find("minpc") != temp.npos) {
      sscanf(&str[idx+1], "%llx", &v1);
      minpc = v1;
    }
    else if(temp.find("tracectrl") != temp.npos) {
      sscanf(&str[idx+1], "%lld %lld %lld", &v1, &v2, &v3);
      TraceCtrl ctrl;
      for(int i=0; i<v3; i++) {
        ctrl.start = v1 + v2*i;
        ctrl.end = ctrl.start + v2;
        tracectrls.push_back(ctrl);
      }
    }
    else if(temp.find("ckptstart") != temp.npos) {
      sscanf(&str[idx+1], "%lld", &v1);
      ckptstart = v1;
    }
    else if(temp.find("logprefix") != temp.npos) {
      idx = idx + 1;
      for(; idx<strlen(str) && str[idx] ==' '; idx++);
      strcpy(logname,  &str[idx]);
    }
    strcpy(str, "");
  }
  fclose(p);

  sort(tracectrls.begin(), tracectrls.end(), cmp);
  printf("----------------- trace settings ------------------- \n");
  printf("record pc: (0x%llx, 0x%llx)\n", minpc, maxpc);
  for(int i=0;i<tracectrls.size();i++) {
    printf("trace interval (raw start: %lld): (%lld %lld)\n", ckptstart, tracectrls[i].start, tracectrls[i].end);
  }
  printf("----------------- -- -- ------------------- \n");
}


// Trace Format :
// Inst PC 				- 8 bytes
// Inst Type 				- 1 byte
// If load/storeInst
//   Effective Address 			- 8 bytes
//   Access Size (one reg)		- 1 byte
// If branch
//   Taken 				- 1 byte
//   If Taken
//     Target				- 8 bytes
// Num Input Regs 			- 1 byte
// Input Reg Names 			- 1 byte each
// Num Output Regs 			- 1 byte
// Output Reg Names 			- 1 byte each
// Output Reg Values
//   If INT (0 to 31) or FLAG (64) 	- 8 bytes each
//   If SIMD (32 to 63)			- 16 bytes each

// aluInstClass = 0,
// loadInstClass = 1,
// storeInstClass = 2,
// condBranchInstClass = 3,
// uncondDirectBranchInstClass = 4,
// uncondIndirectBranchInstClass = 5,
// fpInstClass = 6,
// slowAluInstClass = 7,
// undefInstClass = 8

// reg name
// vecOffset = 32,
// ccOffset = 64

void save_traceinfo(TraceInfo &info)
{
  if(logfp==NULL){
    return ;
  }

  //Inst PC 				- 8 bytes
  fwrite((char*)&(info.pc), 8, 1, logfp);
  //Inst Type 				- 1 byte
  fwrite((char*)&(info.type), 1, 1, logfp);

  //If load/storeInst: 8 bytes address, 1 byte size
  if(info.type == 1 || info.type ==2) {
    fwrite((char*)&(info.memaddr), 8, 1, logfp);
    fwrite((char*)&(info.memsize), 1, 1, logfp);
  }

  //If branch: 1 byte taken, 8 bytes target
  if(info.isbranch) {
    fwrite((char*)&(info.istaken), 1, 1, logfp);
    // printf("%d, 0x%lx, ", info.istaken, info.target);
    if(info.istaken) {
      fwrite((char*)&(info.target), 8, 1, logfp);
    }
  }

  //Num Input Regs 			- 1 byte
  fwrite((char*)&(info.numSrc), 1, 1, logfp);
  //Input Reg Names 			- 1 byte each
  if(info.numSrc>0){
    fwrite((char*)&(info.sregs[0]), 1, info.numSrc, logfp);
  }

  //Num Output Regs 			- 1 byte
  fwrite(&(info.numDest), 1, 1, logfp);
  if(info.numDest>0){
    //Output Reg Names 			- 1 byte each
    fwrite((char*)&(info.dregs[0]), 1, info.numDest, logfp);
    //Output Reg Values     - 8 bytes each
    fwrite((char*)&(info.dvalues[0]), 8, info.numDest, logfp);
  }
}

void record_traces(SimpleThread *thread, StaticInstPtr inst)
{
  uint64_t nowpc = thread->pcState().pc();
  if(!need_record(nowpc)) {
    return ;
  }

  uint64_t npc = thread->nextInstAddr();
  TraceInfo info;
  info.pc = nowpc;
  info.type = covertInstType(inst);
  
  if(info.type == 1 || info.type ==2) {
    if(info.pc == memInfo.pc) {
      info.memaddr = memInfo.addr;
    }
    else {
      info.memaddr = 0;
    }
    info.memsize = 1;
  }

  info.isbranch = 0;
  info.istaken = 0;
  if(inst->isCondCtrl()) {
    info.isbranch = 1;
    info.istaken = !((npc == nowpc+2) || (npc == nowpc+4));
  }
  else if(inst->isUncondCtrl()) {
    info.isbranch = 1;
    info.istaken = 1;
  }
  info.target = npc;

  info.numSrc = inst->numSrcRegs();
  if(info.numSrc > 4) info.numSrc = 4;
  
  for (int idx = 0; idx < info.numSrc; idx++) {
    const RegId& src_reg = inst->srcRegIdx(idx);
    info.sregs[idx] = src_reg.index();  
  }

  info.numDest = inst->numDestRegs();
  if(info.numDest > 4) info.numDest = 4;

  for (int idx = 0; idx < info.numDest; idx++) {
      const RegId& dst_reg = inst->destRegIdx(idx);
      info.dregs[idx] = dst_reg.index();
      switch (dst_reg.classValue()) {
        case IntRegClass:
          info.dvalues[idx] = thread->readIntReg(dst_reg.index());
          break;
        case FloatRegClass:
          info.dvalues[idx] = thread->readFloatReg(dst_reg.index());
          break;
        default:
          info.dvalues[idx] = 0;
      }
  }

  save_traceinfo(info);
}

bool need_record(uint64_t pc)
{
  if(pc >= minpc && pc <= maxpc) {
    if(exeInstNum%5000000 == 0) {
      printf("executed Inst Num: %ld\n", exeInstNum);
    }

    if(exeInstNum == tracectrls[ctrlidx].start) {
      char filename[512];
      sprintf(filename, "%s_start_%lld_len_%lld.traces", logname, tracectrls[ctrlidx].start + ckptstart, tracectrls[ctrlidx].end-tracectrls[ctrlidx].start);
      printf("create trace log (%d): %s\n", ctrlidx+1, filename);
      logfp = fopen(filename, "wb");
      if(logfp == NULL) {
        printf("%s cannot be write!\n", filename);
        traceOpen = false;
      }
    }

    if(exeInstNum == tracectrls[ctrlidx].end-1) {
      if(logfp != NULL) {
        fclose(logfp);
      }
      ctrlidx++;
      if(ctrlidx == tracectrls.size()){
        printf("target traces is produces\n");
        exit(0);
      }
    }

    if(exeInstNum >= tracectrls[ctrlidx].start && exeInstNum < tracectrls[ctrlidx].end) {
      exeInstNum++;
      return true;
    }
    exeInstNum++;
  }
  return false;
}


void record_meminfo(uint64_t pc, uint64_t addr)
{
    memInfo.pc = pc;
    memInfo.addr = addr;
}


uint8_t covertInstType(StaticInstPtr inst) 
{
  uint8_t type = 8;
  if(inst->isLoad()) type = 1;
  else if(inst->isAtomic()) type = 1;
  else if(inst->isStore()) type = 2;
  else if(inst->isFloating()) type = 6;
  else if(inst->isCondCtrl()) type = 3;
  else if(inst->isUncondCtrl()) {
    if(inst->isDirectCtrl())  type = 4;
    if(inst->isIndirectCtrl()) type = 5;
  }
  else if(inst->isInteger()) {
    type = 0;
    OpClass opcode = inst->opClass();
    if(opcode == IntMultOp || opcode == IntDivOp) {
      type = 7;
    }
  } 
  return type;
}

}