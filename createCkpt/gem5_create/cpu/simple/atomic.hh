/*
 * Copyright (c) 2012-2013, 2015, 2018, 2020 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CPU_SIMPLE_ATOMIC_HH__
#define __CPU_SIMPLE_ATOMIC_HH__

#include "cpu/simple/base.hh"
#include "cpu/simple/exec_context.hh"
#include "mem/request.hh"
#include "params/AtomicSimpleCPU.hh"
#include "sim/probe/probe.hh"
#include "debug/ShowRegInfo.hh"

#include <set>
#include <vector>
using namespace std;

typedef struct{
  uint64_t addr;
  unsigned int size;
}CodeRange;

typedef struct{
  uint64_t addr;
  uint8_t data[8];
  uint8_t first;
  unsigned char size;
}LoadInfo;


class CkptInfo{
public:
  uint64_t startnum, simNums, length, exit_pc, pc, npc;
  uint64_t intregs[32], fpregs[32];
  vector<LoadInfo> loads;
  vector<LoadInfo> firstloads;
  vector<CodeRange> memrange;
  vector<CodeRange> textrange;
  set<uint64_t> preAccess;
  set<uint64_t> textAccess;

  CkptInfo(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc){
    this->startnum = startnum;
    for(int i=0;i<32;i++){
      this->intregs[i] = intregs[i];
      this->fpregs[i] = fpregs[i];
    }
    this->loads.clear();
    this->firstloads.clear();
    this->memrange.clear();
    this->textrange.clear();
    this->preAccess.clear();
    this->textAccess.clear();
    this->length = length;
    this->pc = pc;
    this->npc = npc;
    DPRINTF(ShowRegInfo, "{\"type\": \"startckpt\", \"startnum\": \"%ld\"}\n", startnum);

  }
  ~CkptInfo(){
    this->loads.clear();
    this->preAccess.clear();
    this->textAccess.clear();
    this->firstloads.clear();
    this->memrange.clear();
    this->textrange.clear();
  }

public:
  void addload(uint64_t addr, uint8_t *data, unsigned char size) {
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

  void addstore(uint64_t addr, unsigned char size) {
    for(int i=0; i<size; i++,addr++){
      this->preAccess.insert(addr);
    }
  }

  void addinst(uint64_t addr) {
    for(int i=0; i<4; i++,addr++){
      this->textAccess.insert(addr);
    }
  }

  static bool cmp(LoadInfo &info1, LoadInfo &info2) {
    return info1.addr < info2.addr;
  }

  void combine_loads(vector<LoadInfo> &temploads) {
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

  void getFistloads() {
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

  void getRange(set<uint64_t> &addrs, vector<CodeRange> &ranges) {
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

  bool detectOver(uint64_t exit_place, uint64_t exit_pc) {
    bool isOver = false;
    if(exit_place >= this->startnum + this->length) {
      isOver = true;
      this->exit_pc = exit_pc;
      this->simNums = exit_place - this->startnum;
    }
    if(!isOver) {
      return false;
    }

    this->getFistloads();
    this->getRange(this->preAccess, this->memrange);
    this->getRange(this->textAccess, this->textrange);
    saveCkptInfo();
    return isOver;
  }

  void saveCkptInfo() {
    DPRINTF(ShowRegInfo, "{\"type\": \"ckptinfo\", \"startnum\": \"%ld\", \"exitnum\": \"%ld\", \"length\": \"%ld\", \"pc\": \"0x%lx\", \"npc\": \"0x%lx\", \"exitpc\": \"0x%lx\"", startnum, simNums, length, pc, npc, exit_pc);

    char str[3000];
    sprintf(str, "{\"type\": \"int_regs\", \"data\": [ ");
    for(int i=0;i<31;i++){
        sprintf(str, "%s\"0x%lx\", ", str, intregs[i]);
    }
    DPRINTF(ShowRegInfo, "%s\"0x%lx\" ]}\n", str, intregs[31]);

    sprintf(str, "{\"type\": \"fp_regs\", \"data\": [ ");
    for(int i=0;i<31;i++){
        sprintf(str, "%s\"0x%lx\", ", str, fpregs[i]);
    }
    DPRINTF(ShowRegInfo, "%s\"0x%lx\" ]}\n", str, fpregs[31]);

    sprintf(str, "{\"type\": \"textRange\", \"addr\": [ ");
    int size = textrange.size();
    int i=0;
    for(i=0;i<size-1;i++){
        sprintf(str, "%s\"0x%lx\", ", str, textrange[i].addr);
    }
    sprintf(str, "%s\"0x%lx\" ], \"size\": [ ", str, textrange[size-1].addr);
    for(i=0;i<size-1;i++){
        sprintf(str, "%s\"0x%lx\", ", str, textrange[i].size);
    }
    DPRINTF(ShowRegInfo, "%s\"0x%lx\" ] }\n", str, textrange[size-1].size);

    sprintf(str, "{\"type\": \"memRange\", \"addr\": [ ");
    size = memrange.size();
    for(i=0;i<size-1;i++){
        sprintf(str, "%s\"0x%lx\", ", str, memrange[i].addr);
    }
    sprintf(str, "%s\"0x%lx\" ], \"size\": [ ", str, memrange[size-1].addr);
    for(i=0;i<size-1;i++){
        sprintf(str, "%s\"0x%x\", ", str, memrange[i].size);
    }
    DPRINTF(ShowRegInfo, "%s\"0x%x\" ] }\n", str, memrange[size-1].size);

    for(int i=0;i<firstloads.size();i++) {
      DPRINTF(ShowRegInfo, "{\"type\": \"fld\", \"a\": \"0x%lx\", \"s\": \"0x%x\", \"d\": \"0x%lx\"}\n", firstloads[i].addr, firstloads[i].size, *((uint64_t *)firstloads[i].data));
    }

    printf("ckptinfo, startnum: %ld, exitnum: %ld, length: %ld, pc: 0x%lx, npc: 0x%lx, exitpc: 0x%lx\n", startnum, simNums, length, pc, npc, exit_pc);
    printf("text range num: %ld, mem range num: %ld, first load num: %ld\n", textrange.size(), memrange.size(), firstloads.size());

    preAccess.clear();
    textAccess.clear();
    firstloads.clear();
    memrange.clear();
    textrange.clear();
  }
};



namespace gem5
{

class AtomicSimpleCPU : public BaseSimpleCPU
{
  public:

    AtomicSimpleCPU(const AtomicSimpleCPUParams &params);
    virtual ~AtomicSimpleCPU();

    void init() override;

    uint64_t tempregs[32];
    uint64_t ckpt_startinsts, ckpt_endinsts;

    bool startshow = false;
    bool needshowFirst = false;
    bool startlog = false;

    vector<CkptInfo *> pendingCkpts;
    void addCkpt(uint64_t startnum, uint64_t length, uint64_t intregs[], uint64_t fpregs[], uint64_t pc, uint64_t npc) {
      pendingCkpts.push_back(new CkptInfo(startnum, length, intregs, fpregs, pc, npc));
    }

    void addload(uint64_t addr, uint8_t *data, unsigned char size) {
      for(int i=0;i<pendingCkpts.size();i++){
        pendingCkpts[i]->addload(addr, data, size);
      }
    } 

    void addstore(uint64_t addr, unsigned char size) {
      for(int i=0;i<pendingCkpts.size();i++){
        pendingCkpts[i]->addstore(addr, size);
      }
    } 

    void addinst(uint64_t addr) {
      for(int i=0;i<pendingCkpts.size();i++){
        pendingCkpts[i]->addinst(addr);
      }
    } 

    void detectOver(uint64_t exit_place, uint64_t exit_pc) {
      for (vector<CkptInfo *>::iterator it = pendingCkpts.begin(); it != pendingCkpts.end();) {
        if ((*it)->detectOver(exit_place, exit_pc)) {
          delete *it;
          it = pendingCkpts.erase(it);
        } else {
          ++it;
        }
      }
    }


    set<Addr> preinsts;

  protected:
    EventFunctionWrapper tickEvent;

    const int width;
    const int ckptinsts;
    bool locked;
    const bool simulate_data_stalls;
    const bool simulate_inst_stalls;

    // main simulation loop (one cycle)
    void tick();

    /**
     * Check if a system is in a drained state.
     *
     * We need to drain if:
     * <ul>
     * <li>We are in the middle of a microcode sequence as some CPUs
     *     (e.g., HW accelerated CPUs) can't be started in the middle
     *     of a gem5 microcode sequence.
     *
     * <li>The CPU is in a LLSC region. This shouldn't normally happen
     *     as these are executed atomically within a single tick()
     *     call. The only way this can happen at the moment is if
     *     there is an event in the PC event queue that affects the
     *     CPU state while it is in an LLSC region.
     *
     * <li>Stay at PC is true.
     * </ul>
     */
    bool
    isCpuDrained() const
    {
        SimpleExecContext &t_info = *threadInfo[curThread];
        return t_info.thread->microPC() == 0 &&
            !locked && !t_info.stayAtPC;
    }

    /**
     * Try to complete a drain request.
     *
     * @returns true if the CPU is drained, false otherwise.
     */
    bool tryCompleteDrain();

    virtual Tick sendPacket(RequestPort &port, const PacketPtr &pkt);
    virtual Tick fetchInstMem();

    /**
     * An AtomicCPUPort overrides the default behaviour of the
     * recvAtomicSnoop and ignores the packet instead of panicking. It
     * also provides an implementation for the purely virtual timing
     * functions and panics on either of these.
     */
    class AtomicCPUPort : public RequestPort
    {

      public:

        AtomicCPUPort(const std::string &_name, BaseSimpleCPU* _cpu)
            : RequestPort(_name, _cpu)
        { }

      protected:

        bool
        recvTimingResp(PacketPtr pkt)
        {
            panic("Atomic CPU doesn't expect recvTimingResp!\n");
        }

        void
        recvReqRetry()
        {
            panic("Atomic CPU doesn't expect recvRetry!\n");
        }

    };

    class AtomicCPUDPort : public AtomicCPUPort
    {

      public:
        AtomicCPUDPort(const std::string &_name, BaseSimpleCPU *_cpu)
            : AtomicCPUPort(_name, _cpu), cpu(_cpu)
        {
            cacheBlockMask = ~(cpu->cacheLineSize() - 1);
        }

        bool isSnooping() const { return true; }

        Addr cacheBlockMask;
      protected:
        BaseSimpleCPU *cpu;

        virtual Tick recvAtomicSnoop(PacketPtr pkt);
        virtual void recvFunctionalSnoop(PacketPtr pkt);
    };


    AtomicCPUPort icachePort;
    AtomicCPUDPort dcachePort;


    RequestPtr ifetch_req;
    RequestPtr data_read_req;
    RequestPtr data_write_req;
    RequestPtr data_amo_req;

    bool dcache_access;
    Tick dcache_latency;

    /** Probe Points. */
    ProbePointArg<std::pair<SimpleThread *, const StaticInstPtr>> *ppCommit;

  protected:

    /** Return a reference to the data port. */
    Port &getDataPort() override { return dcachePort; }

    /** Return a reference to the instruction port. */
    Port &getInstPort() override { return icachePort; }

    /** Perform snoop for other cpu-local thread contexts. */
    void threadSnoop(PacketPtr pkt, ThreadID sender);

  public:

    DrainState drain() override;
    void drainResume() override;

    void switchOut() override;
    void takeOverFrom(BaseCPU *old_cpu) override;

    void verifyMemoryMode() const override;

    void activateContext(ThreadID thread_num) override;
    void suspendContext(ThreadID thread_num) override;

    /**
     * Helper function used to set up the request for a single fragment of a
     * memory access.
     *
     * Takes care of setting up the appropriate byte-enable mask for the
     * fragment, given the mask for the entire memory access.
     *
     * @param req Pointer to the Request object to populate.
     * @param frag_addr Start address of the fragment.
     * @param size Total size of the memory access in bytes.
     * @param flags Request flags.
     * @param byte_enable Byte-enable mask for the entire memory access.
     * @param[out] frag_size Fragment size.
     * @param[in,out] size_left Size left to be processed in the memory access.
     * @return True if the byte-enable mask for the fragment is not all-false.
     */
    bool genMemFragmentRequest(const RequestPtr &req, Addr frag_addr,
                               int size, Request::Flags flags,
                               const std::vector<bool> &byte_enable,
                               int &frag_size, int &size_left) const;

    Fault readMem(Addr addr, uint8_t *data, unsigned size,
                  Request::Flags flags,
                  const std::vector<bool> &byte_enable=std::vector<bool>())
        override;

    Fault
    initiateHtmCmd(Request::Flags flags) override
    {
        panic("initiateHtmCmd() is for timing accesses, and should "
              "never be called on AtomicSimpleCPU.\n");
    }

    void
    htmSendAbortSignal(HtmFailureFaultCause cause) override
    {
        panic("htmSendAbortSignal() is for timing accesses, and should "
              "never be called on AtomicSimpleCPU.\n");
    }

    Fault writeMem(uint8_t *data, unsigned size,
                   Addr addr, Request::Flags flags, uint64_t *res,
                   const std::vector<bool> &byte_enable=std::vector<bool>())
        override;

    Fault amoMem(Addr addr, uint8_t *data, unsigned size,
                 Request::Flags flags, AtomicOpFunctorPtr amo_op) override;

    void regProbePoints() override;

    /**
     * Print state of address in memory system via PrintReq (for
     * debugging).
     */
    void printAddr(Addr a);
};

} // namespace gem5

#endif // __CPU_SIMPLE_ATOMIC_HH__
