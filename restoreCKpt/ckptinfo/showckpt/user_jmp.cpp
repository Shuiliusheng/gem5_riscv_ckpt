#include "ckptinfo.h"
#include <vector>
using namespace std;

vector<MemRangeInfo> freeRanges;
void getRangeInfo(char filename[]) {
    FILE *p = fopen(filename,"rb");
    vector<MemRangeInfo> ranges;
    uint64_t num = 0;
    MemRangeInfo range;
    fread(&num, 8, 1, p);
    for(int i=0;i<num;i++){
        fread(&range, sizeof(MemRangeInfo), 1, p);
        ranges.push_back(range);
    }

    uint64_t offset = (8+num*16) + 5*8 + 64*8;
    fseek(p, offset, SEEK_SET);
    fread(&num, 8, 1, p);
    for(int i=0;i<num;i++){
        fread(&range, sizeof(MemRangeInfo), 1, p);
        ranges.push_back(range);
    }
    fclose(p);

    for(int i=1;i<ranges.size();i++){
        range.addr = ranges[i-1].addr + ranges[i-1].size;
        range.size = ranges[i].addr - range.addr;
        if(range.size > 0) {
            freeRanges.push_back(range);
            // printf("free mem page: (0x%lx 0x%lx) %d KB\n", range.addr, range.addr+range.size, range.size/1024);
        }
    }
    ranges.clear();
}

vector<uint64_t> midpoints;
void findMidPoints(uint64_t base, uint64_t maxtarget)
{
    if(maxtarget < base + MaxJALOffset) {
        return ;
    }
    uint64_t maxaddr = base + MaxJALOffset, jmpmax = 0, temp=0, temp1=0;
    int i = 0;
    while (1) {
        for(; i < freeRanges.size() && (maxaddr >= freeRanges[i].addr); i++);
        temp = freeRanges[i-1].addr + freeRanges[i-1].size - 0x100;
        temp1 = temp + MaxJALOffset;
        if(i==0 || (temp1 < freeRanges[i].addr && temp1 < maxtarget)){
            printf("cannot found jmp path from 0x%x to 0x%x\n", temp1, maxtarget);
            exit(1);
        }
        while(1) {
            jmpmax = (temp > maxaddr) ? maxaddr : temp;
            maxaddr = jmpmax + MaxJALOffset;
            midpoints.push_back(jmpmax);
            if(temp <= maxaddr) break;
            if(maxaddr > maxtarget) break;
        }
        if(maxaddr > maxtarget) break;
    }
    freeRanges.clear();
}


void setJmp(uint64_t instaddr, uint64_t base, uint64_t target) {
    printf("set jmp inst in 0x%lx, jmp from 0x%lx to 0x%lx\n", instaddr, base, target);
}

void replaceSyscall(vector<uint64_t> &pcs) 
{
    printf("\n\n --- set jmp inst for syscall jmp to takeOverSyscall (0x10a46)--- \n");
    setJmp(TPoint2+4, TPoint2+4, TPoint1+4);
    setJmp(TPoint1+4, TPoint1+4, 0x10a46);
    for(int i=1;i<midpoints.size();i++){
        setJmp(midpoints[i]+4, midpoints[i]+4, midpoints[i-1]+4);
    }

    for(int i=1; i<pcs.size(); i++) {
        for(int c=0;c<midpoints.size();c++){
            if(pcs[i] - (midpoints[c]+4) < MaxJALOffset){
                if(i == pcs.size()-1)
                    printf("repace exit inst:  ");
                else
                    printf("repace syscall %d:  ", i-1);
                setJmp(pcs[i], pcs[i], midpoints[c]+4);
            }
        }
    }
}

void produceSysRet(vector<uint64_t> &pcs)
{
    printf("\n\n --- set jmp inst for takeoverSyscall back to syscall+4 --- \n");
    setJmp(TPoint1, TPoint1, TPoint2);

    for(int i=0; i<pcs.size()-1; i++) {
        int max = 0;
        for(max = 0; max < midpoints.size(); max++){
            if(pcs[i]+4 - midpoints[max] < MaxJALOffset) {
                break;
            }
        }
        if(i==0){
            printf("jmp to start npc: ");
        }
        else{
            printf("jmp to syscall %d: ", i-1);
        }
        for(int c=0; c<max; c++) {
            setJmp( midpoints[c],  midpoints[c],  midpoints[c+1]);
        }
        setJmp( midpoints[max],  midpoints[max],  pcs[i]+4);
    }
}

void produceJmpInst(uint64_t npc, uint64_t exitpc, uint64_t syscall_addr)
{
    vector<uint64_t> pcs;
    pcs.push_back(npc-4);

    uint64_t totalcallnum = *((uint64_t *)syscall_addr);
    SyscallInfo *sinfos = NULL;
    for(int i=0; i<totalcallnum; i++) {
        sinfos = (SyscallInfo *)(syscall_addr + 8 + i*sizeof(SyscallInfo));
        pcs.push_back(sinfos->pc);
    }
    pcs.push_back(exitpc);

    uint64_t maxtarget = 0;
    for(int i=0;i<pcs.size();i++){
        if(maxtarget < pcs[i]) maxtarget = pcs[i];
    }
    midpoints.push_back(TPoint2);
    findMidPoints(TPoint2, maxtarget);
    printf("\n\n--- jmp midpoint information ---\n");
    for(int i=0;i<midpoints.size();i++) {
        printf("jmp midpoint %d: 0x%lx\n", i, midpoints[i]);
    }

    replaceSyscall(pcs);
    produceSysRet(pcs);

    pcs.clear();
    midpoints.clear();
}
