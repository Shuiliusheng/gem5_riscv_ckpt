#include "ckptinfo.h"
#include <vector>
#include <set>
#include <map>
#include <algorithm>
using namespace std;


uint32_t createJAL(uint64_t base, uint64_t target) {
    uint32_t offset = target - base;
    uint32_t op = 0x6f, rd = 0, inst = 0;
    uint32_t imm20=0, v12 = 0, v11 = 0, v1 = 0, v20 = 0;
    v1 = (offset >> 1) % 1024;
    v11 = (offset >> 11) % 2;
    v12 = (offset >> 12) % 256;
    v20 = (offset >> 20) % 2;   
    imm20 = v12 + (v11 << 8) + (v1 << 9) + (v20 << 19);
    inst = op + (rd << 7) + (imm20 << 12); 
    return inst;
}

uint32_t createJALR(uint32_t srcreg) {
    uint32_t op = 0x67;//1100111
    uint32_t fun3 = 0x0, imm11=0x0, dstreg = 0x0;
    uint32_t inst = (imm11<<20) + ((srcreg%32) << 15) + (fun3 << 12) + (dstreg << 7) + op;
    return inst;
}

bool setJmp(uint64_t instaddr, uint64_t target) 
{
    *(uint32_t *)instaddr = createJAL(instaddr, target);
    return true;
}

uint32_t createLD(uint32_t dstreg, uint32_t basereg, uint32_t offset) {
    uint32_t opcode = 0x3, fun3 = 0x3; //0000011
    uint32_t inst = 0;
    inst = inst + ((offset % 4096) << 20);
    inst = inst + ((basereg % 32) << 15);
    inst = inst + (fun3 << 12);
    inst = inst + ((dstreg % 32) << 7);
    inst = inst + opcode;
    return inst;
}

void createImm32(uint32_t dstreg, uint32_t imm32, uint32_t &inst1, uint32_t &inst2) {
    uint32_t oplui = 0x37, opaddiw = 0x1b; //0000011, 0011011
    uint32_t fun3_addiw = 0;
    uint32_t imm20 = imm32>>12, imm12 = (imm32<<20) >> 20;
    if((imm12>>11) == 1) imm20++;
    inst1 = (imm20<<12) + ((dstreg%32)<<7) + oplui;
    inst2 = (imm12<<20) + ((dstreg%32)<<15) + (fun3_addiw<<12) + ((dstreg%32)<<7) + opaddiw;
}


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
    fclose(p);

    for(int i=1;i<ranges.size();i++){
        range.addr = ranges[i-1].addr + ranges[i-1].size;
        range.size = ranges[i].addr - range.addr;
        if(ranges[i].addr > text_seg.addr + text_seg.size) break;
        if(range.size > 0) {
            freeRanges.push_back(range);
        }
    }
    ranges.clear();
}

vector<uint64_t> midpoints;
typedef struct {
    uint64_t label;
    uint64_t pc;
    uint64_t midpoint;
}PCs;

bool less_sort_pc(PCs a, PCs b) { return (a.pc < b.pc); }
bool less_sort_label(PCs a, PCs b) { return (a.label < b.label); }


void findMidPoints(vector<PCs> &pcs)
{
    int p = 0, r = 0;    
    uint32_t freemem = 0x40; //0x40: 64B的空余
    sort(pcs.begin(), pcs.end(), less_sort_pc);
    for(p=0; p<pcs.size(); p++) {
        for(r; r<freeRanges.size(); r++) {
            if(pcs[p].pc < freeRanges[r].addr) { 
                int dis = freeRanges[r].addr + freemem - pcs[p].pc;
                if(dis <= MaxJALOffset) {
                    pcs[p].midpoint = freeRanges[r].addr;
                    break;
                }
                else {
                    printf("cannot find midpoint for pc: 0x%lx, freerange: 0x%lx 0x%lx\n", pcs[p].pc, freeRanges[r].addr, freeRanges[r].addr + freeRanges[r].size);
                    exit(0);
                }
            }
            else {
                uint64_t midpoint = (freeRanges[r].addr + freeRanges[r].size - freemem);
                int dis = pcs[p].pc - midpoint;
                if(dis <= MaxJALOffset) {
                    pcs[p].midpoint = midpoint;
                    break;
                }
            }
        }
        if(r==freeRanges.size()) {
            printf("cannot find midpoint for pc: 0x%lx\n", pcs[p].pc);
            exit(0);
        }
    }
    sort(pcs.begin(), pcs.end(), less_sort_label);
    // for(p=0; p<pcs.size(); p++)
    //     printf("pcs %d: 0x%lx, midpoint: 0x%lx, dis: %d\n", p, pcs[p].pc, pcs[p].midpoint, pcs[p].midpoint - pcs[p].pc);
}

void createInsts_jmp2prog(vector<PCs> &pcs, uint32_t memaddr)
{
    JmpRepInfo *jmpinfos = runningInfo.sysJmpinfos;
    uint32_t jal_inst, imm_inst1, imm_inst2, ld_inst;
    uint64_t ret_addr, free_addr;
    for(int i=0; i<pcs.size()-1; i++) {
        ret_addr = pcs[i].pc + 4;
        if(i==0)
            ret_addr = pcs[i].pc;
        free_addr = pcs[i].midpoint + 32;

        createImm32(10, memaddr, imm_inst1, imm_inst2);
        ld_inst = createLD(10, 10, 0);
        jal_inst = createJAL(free_addr+12, ret_addr);

        *((uint32_t *)(free_addr+0)) = imm_inst1;   //set imm
        *((uint32_t *)(free_addr+4)) = imm_inst2;
        *((uint32_t *)(free_addr+8)) = ld_inst;     //load a0

        jmpinfos[i].jalr_target = free_addr;
        jmpinfos[i].jal_addr = free_addr+12;
        jmpinfos[i].jal_inst = jal_inst;
        // printf("jaladdr: 0x%lx, inst: 0x%lx, target: 0x%lx\n", jmpinfos[i].jal_addr, jmpinfos[i].jal_inst, ret_addr);
    }
}

void createInsts_jmp2loader(vector<PCs> &pcs, uint32_t memaddr)
{
    uint32_t jal_inst, imm_inst1, imm_inst2, ld_inst, jalr_inst;
    uint64_t ecall_addr, free_addr;
    for(int i=1; i<pcs.size(); i++) {
        ecall_addr = pcs[i].pc;
        free_addr = pcs[i].midpoint;

        //produce inst to jmp to midpoint and get addr to takeOverfunc and jmp
        jal_inst = createJAL(ecall_addr, free_addr);
        createImm32(10, memaddr, imm_inst1, imm_inst2);
        ld_inst = createLD(10, 10, 0);
        jalr_inst = createJALR(10);

        //replace inst
        *((uint32_t *)(free_addr+0)) = imm_inst1;   //set imm
        *((uint32_t *)(free_addr+4)) = imm_inst2;
        *((uint32_t *)(free_addr+8)) = ld_inst;     //load takeOverAddr
        *((uint32_t *)(free_addr+12)) = jalr_inst;  //jalr to takeOverFunc
        if(i<pcs.size()-1) 
            *((uint32_t *)ecall_addr) = jal_inst;   //replace syscall
        else
            runningInfo.exitJmpInst = jal_inst;
    }
}

void processJmp(uint64_t npc)
{
    vector<PCs> pcs;
    PCs data;
    data.label = 0;
    data.pc = npc;
    pcs.push_back(data);
    RunningInfo *runinfo = (RunningInfo *)&runningInfo;
    SyscallInfo *sinfos = NULL;
    uint64_t infoaddr = runinfo->syscall_info_addr + 8 + runinfo->totalcallnum*4;
    uint32_t *sysidxs = (uint32_t *)(runinfo->syscall_info_addr + 8);
    int diffcallnum = -1;
    for(int i=0; i<runinfo->totalcallnum; i++) {
        if((int)sysidxs[i] > diffcallnum) 
            diffcallnum = sysidxs[i];
    }
    for(int i=0; i<diffcallnum+1; i++) {
        sinfos = (SyscallInfo *)(infoaddr + i*sizeof(SyscallInfo));
        data.label = i+1;
        data.pc = sinfos->pc;
        pcs.push_back(data);
        // printf("sysidx %d: 0x%lx\n", i, data.pc);
    }
    data.label = runinfo->totalcallnum+1;
    data.pc = runinfo->exitpc;
    pcs.push_back(data);

    findMidPoints(pcs);

    uint64_t takeOverAddr_place = (uint64_t) &takeOverAddr;
    uint64_t a0_place = (uint64_t) &program_intregs[10];

    runningInfo.sysJmpinfos = (JmpRepInfo *)malloc(sizeof(JmpRepInfo)*pcs.size());
    createInsts_jmp2loader(pcs, takeOverAddr_place);
    createInsts_jmp2prog(pcs, a0_place);
    pcs.clear();
}



void updateJmpInst(JmpRepInfo &info)
{
    // printf("update inst, jalr_target: 0x%lx, jal_addr: 0x%lx\n", info.jalr_target, info.jal_addr);
    tempMemory[0] = info.jalr_target;
    *(uint32_t *)(info.jal_addr) = info.jal_inst;
    asm volatile("fence.i");
}