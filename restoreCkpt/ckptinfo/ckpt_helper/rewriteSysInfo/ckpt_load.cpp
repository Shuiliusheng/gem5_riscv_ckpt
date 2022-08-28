#include "ckptinfo.h"
#include<vector>
#include<set>
using namespace std;

typedef struct{
    uint64_t addr;
    // uint64_t size;
    uint64_t data;
}LoadInfo;
//默认均为8B

typedef struct{
    uint64_t start;
    uint64_t simNum;
    uint64_t exitpc;
    uint64_t exit_cause;
}SimInfo;

uint64_t alloc_vaddr;

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

    if(info1->p0 != info2->p0) 
        return false;

    if(info1->data_size == 0)
        return true;

    if(info1->data_size != info2->data_size) 
        return false;

    if(info1->bufaddr != info2->bufaddr) 
        return false;

    unsigned char *data1 = (unsigned char *)(alloc_vaddr + info1->data_offset);
    unsigned char *data2 = (unsigned char *)(alloc_vaddr + info2->data_offset);
    if(memcmp(data1, data2, info1->data_size)!=0)
        return false;

    // printf("same data, pc: 0x%lx, num: %d, size: %d\n", info1->pc, info1->num, info1->data_size);
    return true;
}


uint32_t dectect_Sysinfos(SyscallInfo *sysinfos, uint32_t idx, set<uint64_t> &prepc, set<uint32_t> &prenum, vector<uint32_t> &newinfos)
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


void process(uint64_t totalnum, FILE *p)
{
    vector<uint32_t> sys_idxs;
    vector<uint32_t> newsysinfos, newsysinfos1; //no data
    set<uint64_t> prepc, prepc1;
    set<uint32_t> prenum, prenum1;
    uint32_t num1 = 0;
    SyscallInfo *sysinfos = (SyscallInfo *)(alloc_vaddr + 8);

    for(int i=0;i<totalnum;i++) {
        if(sysinfos[i].data_size != 0){
            sys_idxs.push_back(dectect_Sysinfos(sysinfos, i, prepc1, prenum1, newsysinfos1));
            num1 ++;
        }
        else{
            sys_idxs.push_back(dectect_Sysinfos(sysinfos, i, prepc, prenum, newsysinfos));
        }
    }
    int size1 = newsysinfos.size();
    for(int i=0;i<totalnum;i++) {
        if(sysinfos[i].data_size != 0) {
            sys_idxs[i] += size1;
        }
    }
    printf("before syscall num: (%d %d), after num: (%d %d)\n", num1, totalnum-num1, newsysinfos1.size(), size1);
    uint64_t idx_size = 4*totalnum;
    uint64_t info_size = 5*8*(newsysinfos.size()+newsysinfos1.size());
    uint64_t data_addr = idx_size + info_size + 8; 
    uint64_t invalid_addr = 0xffffffff; 
    fwrite(&totalnum, sizeof(uint64_t), 1, p);
    fwrite(&sys_idxs[0], sizeof(uint32_t), totalnum, p);

    uint64_t mem[10];
    for(int i=0; i<newsysinfos.size(); i++) {
        int idx = newsysinfos[i];
        mem[0] = sysinfos[idx].pc;
        mem[1] = (sysinfos[idx].num << 32) + sysinfos[idx].data_size;
        mem[2] = sysinfos[idx].p0;
        mem[3] = sysinfos[idx].ret;
        mem[4] = (sysinfos[idx].hasret) ? (invalid_addr<<8) + 1 : (invalid_addr<<8) + 0;
        fwrite(&mem[0], sizeof(uint64_t), 5, p);
    }

    for(int i=0; i<newsysinfos1.size(); i++) {
        int idx = newsysinfos1[i];
        mem[0] = sysinfos[idx].pc;
        mem[1] = (sysinfos[idx].num << 32) + sysinfos[idx].data_size;
        mem[2] = sysinfos[idx].p0;
        mem[3] = sysinfos[idx].ret;
        mem[4] = (sysinfos[idx].hasret) ? (data_addr<<8) + 1 : (data_addr<<8) + 0;
        fwrite(&mem[0], sizeof(uint64_t), 5, p);
        data_addr = data_addr + sysinfos[idx].data_size + sizeof(uint64_t);  
    }

    for(int i=0; i<newsysinfos1.size(); i++) {
        int idx = newsysinfos1[i];
        unsigned char *data = (unsigned char *)(alloc_vaddr + sysinfos[idx].data_offset);
        fwrite(&sysinfos[idx].bufaddr, sizeof(uint64_t), 1, p);
        fwrite(data, 1, sysinfos[idx].data_size, p);
    }
}



uint64_t read_ckptsyscall(FILE *fp, FILE *fq)
{
    uint64_t filesize, allocsize, nowplace;
    nowplace = ftell(fp);
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp) - nowplace;
    fseek(fp, nowplace, SEEK_SET);

    allocsize = filesize + (4096-filesize%4096);
    alloc_vaddr = (uint64_t)mmap((void *)0x2000000, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);    
    
    fread((void *)alloc_vaddr, filesize, 1, fp);
    uint64_t totalcallnum = *((uint64_t *)alloc_vaddr);
    printf("--- syscall totalcallnum: %ld ---, filesize: %d KB\n", totalcallnum, filesize/1024);
    process(totalcallnum, fq);
    return alloc_vaddr;
}

FILE *open_newCKpt(char ckptinfo[])
{
    char filename[300];
    int len = strlen(ckptinfo);
    int idx = len-1;
    for(; idx>=0 && (ckptinfo[idx]!='.'); idx--);
    if(idx > 0) ckptinfo[idx] = '\0';
    sprintf(filename,"%s_new.info", ckptinfo);
    printf("dst filename: %s\n", filename);
    FILE *p =fopen(filename, "wb");
    return p;
}


void read_ckptinfo(char ckptinfo[])
{
    uint64_t npc=0, temp=0, numinfos=0;
    SimInfo siminfo;
    FILE *p = fopen(ckptinfo,"rb");
    if(p == NULL){
        printf("cannot open %s to read\n", ckptinfo);
        exit(1);
    }
    FILE *q = open_newCKpt(ckptinfo);

    fread(&numinfos, 8, 1, p);
    fwrite(&numinfos, 8, 1, q);

    temp = 0;
    MemRangeInfo textinfo;
    for(int i=0;i<numinfos;i++){
        fread(&textinfo, sizeof(MemRangeInfo), 1, p);
        fwrite(&textinfo, sizeof(MemRangeInfo), 1, q);

        temp += textinfo.size;
    }
    printf("text range num: %d, total size: %ld KB\n", numinfos, temp/1024);


    fread(&siminfo, sizeof(siminfo), 1, p);
    fwrite(&siminfo, sizeof(siminfo), 1, q);

    //step 1: read npc
    fread(&npc, 8, 1, p);
    fwrite(&npc, 8, 1, q);

    uint64_t regs[64];
    fread(&regs[0], 8, 64, p);
    fwrite(&regs[0], 8, 64, q);

    //step 3: read memory range information and map these ranges
    MemRangeInfo memrange;
    temp = 0;
    fread(&numinfos, 8, 1, p);
    fwrite(&numinfos, 8, 1, q);
    for(int i=0;i<numinfos;i++){
        fread(&memrange, sizeof(MemRangeInfo), 1, p);
        fwrite(&memrange, sizeof(MemRangeInfo), 1, q);
        temp += memrange.size;
    }
    printf("mem range num: %d, total size: %ld KB\n", numinfos, temp/1024);

    //step 4: read first load information, and store these data to memory
    fread(&numinfos, 8, 1, p);
    fwrite(&numinfos, 8, 1, q);
    printf("--- first load num: %ld ---\n", numinfos);

    LoadInfo linfos;
    for(int i=0;i<numinfos;i++){
        fread(&linfos, sizeof(LoadInfo), 1, p);
        fwrite(&linfos, sizeof(LoadInfo), 1, q);
    }
    
    //step5: 加载syscall的执行信息到内存中
    uint64_t alloc_vaddr = read_ckptsyscall(p, q);
    fclose(p);
    fclose(q);
}
