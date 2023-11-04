#ifndef __SIM_TRACE_COLLECT_HH__
#define __SIM_TRACE_COLLECT_HH__

#include "cpu/simple/base.hh"
#include "cpu/simple_thread.hh"
#include "cpu/reg_class.hh"
#include "cpu/op_class.hh"
#include "cpu/static_inst_fwd.hh"

#include <string>
#include <vector>
#include <set>
using namespace std;

typedef struct{
    uint64_t pc, addr;
}MemAddrinfo;

typedef struct{
    uint64_t pc;
    uint8_t type;
    uint64_t memaddr;
    uint8_t memsize;
    uint8_t isbranch;
    uint8_t istaken;
    uint64_t target;
    uint8_t numSrc;
    uint8_t sregs[4];
    uint8_t numDest;
    uint8_t dregs[4];
    uint64_t dvalues[4];
}TraceInfo;

namespace gem5
{

void init_trace_settings(const char filename[]);
void record_meminfo(uint64_t pc, uint64_t addr);
void record_traces(SimpleThread *thread, StaticInstPtr inst);


bool need_record(uint64_t pc);
uint8_t covertInstType(StaticInstPtr inst);
void save_traceinfo(TraceInfo &info);

extern bool traceOpen;

}

#endif