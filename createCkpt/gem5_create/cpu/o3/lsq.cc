/*
 * Copyright (c) 2011-2012, 2014, 2017-2019 ARM Limited
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
 * All rights reserved
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
 * Copyright (c) 2005-2006 The Regents of The University of Michigan
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

#include "cpu/o3/lsq.hh"

#include <algorithm>
#include <list>
#include <string>

#include "base/compiler.hh"
#include "base/logging.hh"
#include "cpu/o3/cpu.hh"
#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/iew.hh"
#include "cpu/o3/limits.hh"
#include "debug/Drain.hh"
#include "debug/Fetch.hh"
#include "debug/HtmCpu.hh"
#include "debug/LSQ.hh"
#include "debug/Writeback.hh"
#include "params/O3CPU.hh"

namespace gem5
{

namespace o3
{

LSQ::LSQSenderState::LSQSenderState(LSQRequest *request, bool is_load) :
    _request(request), isLoad(is_load), needWB(is_load)
{}

ContextID
LSQ::LSQSenderState::contextId()
{
    return inst->contextId();
}

LSQ::DcachePort::DcachePort(LSQ *_lsq, CPU *_cpu) :
    RequestPort(_cpu->name() + ".dcache_port", _cpu), lsq(_lsq), cpu(_cpu)
{}

LSQ::LSQ(CPU *cpu_ptr, IEW *iew_ptr, const O3CPUParams &params)
    : cpu(cpu_ptr), iewStage(iew_ptr),
      _cacheBlocked(false),
      cacheStorePorts(params.cacheStorePorts), usedStorePorts(0),
      cacheLoadPorts(params.cacheLoadPorts), usedLoadPorts(0),
      lsqPolicy(params.smtLSQPolicy),
      LQEntries(params.LQEntries),
      SQEntries(params.SQEntries),
      maxLQEntries(maxLSQAllocation(lsqPolicy, LQEntries, params.numThreads,
                  params.smtLSQThreshold)),
      maxSQEntries(maxLSQAllocation(lsqPolicy, SQEntries, params.numThreads,
                  params.smtLSQThreshold)),
      dcachePort(this, cpu_ptr),
      numThreads(params.numThreads)
{
    assert(numThreads > 0 && numThreads <= MaxThreads);

    //**********************************************
    //************ Handle SMT Parameters ***********
    //**********************************************

    /* Run SMT olicy checks. */
        if (lsqPolicy == SMTQueuePolicy::Dynamic) {
        DPRINTF(LSQ, "LSQ sharing policy set to Dynamic\n");
    } else if (lsqPolicy == SMTQueuePolicy::Partitioned) {
        DPRINTF(Fetch, "LSQ sharing policy set to Partitioned: "
                "%i entries per LQ | %i entries per SQ\n",
                maxLQEntries,maxSQEntries);
    } else if (lsqPolicy == SMTQueuePolicy::Threshold) {

        assert(params.smtLSQThreshold > params.LQEntries);
        assert(params.smtLSQThreshold > params.SQEntries);

        DPRINTF(LSQ, "LSQ sharing policy set to Threshold: "
                "%i entries per LQ | %i entries per SQ\n",
                maxLQEntries,maxSQEntries);
    } else {
        panic("Invalid LSQ sharing policy. Options are: Dynamic, "
                    "Partitioned, Threshold");
    }

    thread.reserve(numThreads);
    for (ThreadID tid = 0; tid < numThreads; tid++) {
        thread.emplace_back(maxLQEntries, maxSQEntries);
        thread[tid].init(cpu, iew_ptr, params, this, tid);
        thread[tid].setDcachePort(&dcachePort);
    }
}


std::string
LSQ::name() const
{
    return iewStage->name() + ".lsq";
}

void
LSQ::setActiveThreads(std::list<ThreadID> *at_ptr)
{
    activeThreads = at_ptr;
    assert(activeThreads != 0);
}

void
LSQ::drainSanityCheck() const
{
    assert(isDrained());

    for (ThreadID tid = 0; tid < numThreads; tid++)
        thread[tid].drainSanityCheck();
}

bool
LSQ::isDrained() const
{
    bool drained(true);

    if (!lqEmpty()) {
        DPRINTF(Drain, "Not drained, LQ not empty.\n");
        drained = false;
    }

    if (!sqEmpty()) {
        DPRINTF(Drain, "Not drained, SQ not empty.\n");
        drained = false;
    }

    return drained;
}

void
LSQ::takeOverFrom()
{
    usedStorePorts = 0;
    _cacheBlocked = false;

    for (ThreadID tid = 0; tid < numThreads; tid++) {
        thread[tid].takeOverFrom();
    }
}

void
LSQ::tick()
{
    // Re-issue loads which got blocked on the per-cycle load ports limit.
    if (usedLoadPorts == cacheLoadPorts && !_cacheBlocked)
        iewStage->cacheUnblocked();

    usedLoadPorts = 0;
    usedStorePorts = 0;
}

bool
LSQ::cacheBlocked() const
{
    return _cacheBlocked;
}

void
LSQ::cacheBlocked(bool v)
{
    _cacheBlocked = v;
}

bool
LSQ::cachePortAvailable(bool is_load) const
{
    bool ret;
    if (is_load) {
        ret  = usedLoadPorts < cacheLoadPorts;
    } else {
        ret  = usedStorePorts < cacheStorePorts;
    }
    return ret;
}

void
LSQ::cachePortBusy(bool is_load)
{
    assert(cachePortAvailable(is_load));
    if (is_load) {
        usedLoadPorts++;
    } else {
        usedStorePorts++;
    }
}

void
LSQ::insertLoad(const DynInstPtr &load_inst)
{
    ThreadID tid = load_inst->threadNumber;

    thread[tid].insertLoad(load_inst);
}

void
LSQ::insertStore(const DynInstPtr &store_inst)
{
    ThreadID tid = store_inst->threadNumber;

    thread[tid].insertStore(store_inst);
}

Fault
LSQ::executeLoad(const DynInstPtr &inst)
{
    ThreadID tid = inst->threadNumber;

    return thread[tid].executeLoad(inst);
}

Fault
LSQ::executeStore(const DynInstPtr &inst)
{
    ThreadID tid = inst->threadNumber;

    return thread[tid].executeStore(inst);
}

void
LSQ::commitLoads(InstSeqNum &youngest_inst, ThreadID tid)
{
    thread.at(tid).commitLoads(youngest_inst);
}

void
LSQ::commitStores(InstSeqNum &youngest_inst, ThreadID tid)
{
    thread.at(tid).commitStores(youngest_inst);
}

void
LSQ::writebackStores()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (numStoresToWB(tid) > 0) {
            DPRINTF(Writeback,"[tid:%i] Writing back stores. %i stores "
                "available for Writeback.\n", tid, numStoresToWB(tid));
        }

        thread[tid].writebackStores();
    }
}

void
LSQ::squash(const InstSeqNum &squashed_num, ThreadID tid)
{
    thread.at(tid).squash(squashed_num);
}

bool
LSQ::violation()
{
    /* Answers: Does Anybody Have a Violation?*/
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (thread[tid].violation())
            return true;
    }

    return false;
}

bool LSQ::violation(ThreadID tid) { return thread.at(tid).violation(); }

DynInstPtr
LSQ::getMemDepViolator(ThreadID tid)
{
    return thread.at(tid).getMemDepViolator();
}

int
LSQ::getLoadHead(ThreadID tid)
{
    return thread.at(tid).getLoadHead();
}

InstSeqNum
LSQ::getLoadHeadSeqNum(ThreadID tid)
{
    return thread.at(tid).getLoadHeadSeqNum();
}

int
LSQ::getStoreHead(ThreadID tid)
{
    return thread.at(tid).getStoreHead();
}

InstSeqNum
LSQ::getStoreHeadSeqNum(ThreadID tid)
{
    return thread.at(tid).getStoreHeadSeqNum();
}

int LSQ::getCount(ThreadID tid) { return thread.at(tid).getCount(); }

int LSQ::numLoads(ThreadID tid) { return thread.at(tid).numLoads(); }

int LSQ::numStores(ThreadID tid) { return thread.at(tid).numStores(); }

int
LSQ::numHtmStarts(ThreadID tid) const
{
    if (tid == InvalidThreadID)
        return 0;
    else
        return thread[tid].numHtmStarts();
}
int
LSQ::numHtmStops(ThreadID tid) const
{
    if (tid == InvalidThreadID)
        return 0;
    else
        return thread[tid].numHtmStops();
}

void
LSQ::resetHtmStartsStops(ThreadID tid)
{
    if (tid != InvalidThreadID)
        thread[tid].resetHtmStartsStops();
}

uint64_t
LSQ::getLatestHtmUid(ThreadID tid) const
{
    if (tid == InvalidThreadID)
        return 0;
    else
        return thread[tid].getLatestHtmUid();
}

void
LSQ::setLastRetiredHtmUid(ThreadID tid, uint64_t htmUid)
{
    if (tid != InvalidThreadID)
        thread[tid].setLastRetiredHtmUid(htmUid);
}

void
LSQ::recvReqRetry()
{
    iewStage->cacheUnblocked();
    cacheBlocked(false);

    for (ThreadID tid : *activeThreads) {
        thread[tid].recvRetry();
    }
}

void
LSQ::completeDataAccess(PacketPtr pkt)
{
    auto senderState = dynamic_cast<LSQSenderState*>(pkt->senderState);
    thread[cpu->contextToThread(senderState->contextId())]
        .completeDataAccess(pkt);
}

bool
LSQ::recvTimingResp(PacketPtr pkt)
{
    if (pkt->isError())
        DPRINTF(LSQ, "Got error packet back for address: %#X\n",
                pkt->getAddr());

    auto senderState = dynamic_cast<LSQSenderState*>(pkt->senderState);
    panic_if(!senderState, "Got packet back with unknown sender state\n");

    thread[cpu->contextToThread(senderState->contextId())].recvTimingResp(pkt);

    if (pkt->isInvalidate()) {
        // This response also contains an invalidate; e.g. this can be the case
        // if cmd is ReadRespWithInvalidate.
        //
        // The calling order between completeDataAccess and checkSnoop matters.
        // By calling checkSnoop after completeDataAccess, we ensure that the
        // fault set by checkSnoop is not lost. Calling writeback (more
        // specifically inst->completeAcc) in completeDataAccess overwrites
        // fault, and in case this instruction requires squashing (as
        // determined by checkSnoop), the ReExec fault set by checkSnoop would
        // be lost otherwise.

        DPRINTF(LSQ, "received invalidation with response for addr:%#x\n",
                pkt->getAddr());

        for (ThreadID tid = 0; tid < numThreads; tid++) {
            thread[tid].checkSnoop(pkt);
        }
    }
    // Update the LSQRequest state (this may delete the request)
    senderState->request()->packetReplied();

    return true;
}

void
LSQ::recvTimingSnoopReq(PacketPtr pkt)
{
    DPRINTF(LSQ, "received pkt for addr:%#x %s\n", pkt->getAddr(),
            pkt->cmdString());

    // must be a snoop
    if (pkt->isInvalidate()) {
        DPRINTF(LSQ, "received invalidation for addr:%#x\n",
                pkt->getAddr());
        for (ThreadID tid = 0; tid < numThreads; tid++) {
            thread[tid].checkSnoop(pkt);
        }
    }
}

int
LSQ::getCount()
{
    unsigned total = 0;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        total += getCount(tid);
    }

    return total;
}

int
LSQ::numLoads()
{
    unsigned total = 0;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        total += numLoads(tid);
    }

    return total;
}

int
LSQ::numStores()
{
    unsigned total = 0;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        total += thread[tid].numStores();
    }

    return total;
}

unsigned
LSQ::numFreeLoadEntries()
{
    unsigned total = 0;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        total += thread[tid].numFreeLoadEntries();
    }

    return total;
}

unsigned
LSQ::numFreeStoreEntries()
{
    unsigned total = 0;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        total += thread[tid].numFreeStoreEntries();
    }

    return total;
}

unsigned
LSQ::numFreeLoadEntries(ThreadID tid)
{
        return thread[tid].numFreeLoadEntries();
}

unsigned
LSQ::numFreeStoreEntries(ThreadID tid)
{
        return thread[tid].numFreeStoreEntries();
}

bool
LSQ::isFull()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (!(thread[tid].lqFull() || thread[tid].sqFull()))
            return false;
    }

    return true;
}

bool
LSQ::isFull(ThreadID tid)
{
    //@todo: Change to Calculate All Entries for
    //Dynamic Policy
    if (lsqPolicy == SMTQueuePolicy::Dynamic)
        return isFull();
    else
        return thread[tid].lqFull() || thread[tid].sqFull();
}

bool
LSQ::isEmpty() const
{
    return lqEmpty() && sqEmpty();
}

bool
LSQ::lqEmpty() const
{
    std::list<ThreadID>::const_iterator threads = activeThreads->begin();
    std::list<ThreadID>::const_iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (!thread[tid].lqEmpty())
            return false;
    }

    return true;
}

bool
LSQ::sqEmpty() const
{
    std::list<ThreadID>::const_iterator threads = activeThreads->begin();
    std::list<ThreadID>::const_iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (!thread[tid].sqEmpty())
            return false;
    }

    return true;
}

bool
LSQ::lqFull()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (!thread[tid].lqFull())
            return false;
    }

    return true;
}

bool
LSQ::lqFull(ThreadID tid)
{
    //@todo: Change to Calculate All Entries for
    //Dynamic Policy
    if (lsqPolicy == SMTQueuePolicy::Dynamic)
        return lqFull();
    else
        return thread[tid].lqFull();
}

bool
LSQ::sqFull()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (!sqFull(tid))
            return false;
    }

    return true;
}

bool
LSQ::sqFull(ThreadID tid)
{
     //@todo: Change to Calculate All Entries for
    //Dynamic Policy
    if (lsqPolicy == SMTQueuePolicy::Dynamic)
        return sqFull();
    else
        return thread[tid].sqFull();
}

bool
LSQ::isStalled()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (!thread[tid].isStalled())
            return false;
    }

    return true;
}

bool
LSQ::isStalled(ThreadID tid)
{
    if (lsqPolicy == SMTQueuePolicy::Dynamic)
        return isStalled();
    else
        return thread[tid].isStalled();
}

bool
LSQ::hasStoresToWB()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (hasStoresToWB(tid))
            return true;
    }

    return false;
}

bool
LSQ::hasStoresToWB(ThreadID tid)
{
    return thread.at(tid).hasStoresToWB();
}

int
LSQ::numStoresToWB(ThreadID tid)
{
    return thread.at(tid).numStoresToWB();
}

bool
LSQ::willWB()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (willWB(tid))
            return true;
    }

    return false;
}

bool
LSQ::willWB(ThreadID tid)
{
    return thread.at(tid).willWB();
}

void
LSQ::dumpInsts() const
{
    std::list<ThreadID>::const_iterator threads = activeThreads->begin();
    std::list<ThreadID>::const_iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        thread[tid].dumpInsts();
    }
}

void
LSQ::dumpInsts(ThreadID tid) const
{
    thread.at(tid).dumpInsts();
}

Fault
LSQ::pushRequest(const DynInstPtr& inst, bool isLoad, uint8_t *data,
        unsigned int size, Addr addr, Request::Flags flags, uint64_t *res,
        AtomicOpFunctorPtr amo_op, const std::vector<bool>& byte_enable)
{
    // This comming request can be either load, store or atomic.
    // Atomic request has a corresponding pointer to its atomic memory
    // operation
    GEM5_VAR_USED bool isAtomic = !isLoad && amo_op;

    ThreadID tid = cpu->contextToThread(inst->contextId());
    auto cacheLineSize = cpu->cacheLineSize();
    bool needs_burst = transferNeedsBurst(addr, size, cacheLineSize);
    LSQRequest* req = nullptr;

    // Atomic requests that access data across cache line boundary are
    // currently not allowed since the cache does not guarantee corresponding
    // atomic memory operations to be executed atomically across a cache line.
    // For ISAs such as x86 that supports cross-cache-line atomic instructions,
    // the cache needs to be modified to perform atomic update to both cache
    // lines. For now, such cross-line update is not supported.
    assert(!isAtomic || (isAtomic && !needs_burst));

    const bool htm_cmd = isLoad && (flags & Request::HTM_CMD);

    if (inst->translationStarted()) {
        req = inst->savedReq;
        assert(req);
    } else {
        if (htm_cmd) {
            assert(addr == 0x0lu);
            assert(size == 8);
            req = new HtmCmdRequest(&thread[tid], inst, flags);
        } else if (needs_burst) {
            req = new SplitDataRequest(&thread[tid], inst, isLoad, addr,
                    size, flags, data, res);
        } else {
            req = new SingleDataRequest(&thread[tid], inst, isLoad, addr,
                    size, flags, data, res, std::move(amo_op));
        }
        assert(req);
        req->_byteEnable = byte_enable;
        inst->setRequest();
        req->taskId(cpu->taskId());

        // There might be fault from a previous execution attempt if this is
        // a strictly ordered load
        inst->getFault() = NoFault;

        req->initiateTranslation();
    }

    /* This is the place were instructions get the effAddr. */
    if (req->isTranslationComplete()) {
        if (req->isMemAccessRequired()) {
            inst->effAddr = req->getVaddr();
            inst->effSize = size;
            inst->effAddrValid(true);

            if (cpu->checker) {
                inst->reqToVerify = std::make_shared<Request>(*req->request());
            }
            Fault fault;
            if (isLoad)
                fault = cpu->read(req, inst->lqIdx);
            else
                fault = cpu->write(req, data, inst->sqIdx);
            // inst->getFault() may have the first-fault of a
            // multi-access split request at this point.
            // Overwrite that only if we got another type of fault
            // (e.g. re-exec).
            if (fault != NoFault)
                inst->getFault() = fault;
        } else if (isLoad) {
            inst->setMemAccPredicate(false);
            // Commit will have to clean up whatever happened.  Set this
            // instruction as executed.
            inst->setExecuted();
        }
    }

    if (inst->traceData)
        inst->traceData->setMem(addr, size, flags);

    return inst->getFault();
}

void
LSQ::SingleDataRequest::finish(const Fault &fault, const RequestPtr &req,
        gem5::ThreadContext* tc, BaseMMU::Mode mode)
{
    _fault.push_back(fault);
    numInTranslationFragments = 0;
    numTranslatedFragments = 1;
    /* If the instruction has been squahsed, let the request know
     * as it may have to self-destruct. */
    if (_inst->isSquashed()) {
        squashTranslation();
    } else {
        _inst->strictlyOrdered(req->isStrictlyOrdered());

        flags.set(Flag::TranslationFinished);
        if (fault == NoFault) {
            _inst->physEffAddr = req->getPaddr();
            _inst->memReqFlags = req->getFlags();
            if (req->isCondSwap()) {
                assert(_res);
                req->setExtraData(*_res);
            }
            setState(State::Request);
        } else {
            setState(State::Fault);
        }

        LSQRequest::_inst->fault = fault;
        LSQRequest::_inst->translationCompleted(true);
    }
}

void
LSQ::SplitDataRequest::finish(const Fault &fault, const RequestPtr &req,
        gem5::ThreadContext* tc, BaseMMU::Mode mode)
{
    int i;
    for (i = 0; i < _requests.size() && _requests[i] != req; i++);
    assert(i < _requests.size());
    _fault[i] = fault;

    numInTranslationFragments--;
    numTranslatedFragments++;

    if (fault == NoFault)
        mainReq->setFlags(req->getFlags());

    if (numTranslatedFragments == _requests.size()) {
        if (_inst->isSquashed()) {
            squashTranslation();
        } else {
            _inst->strictlyOrdered(mainReq->isStrictlyOrdered());
            flags.set(Flag::TranslationFinished);
            _inst->translationCompleted(true);

            for (i = 0; i < _fault.size() && _fault[i] == NoFault; i++);
            if (i > 0) {
                _inst->physEffAddr = request(0)->getPaddr();
                _inst->memReqFlags = mainReq->getFlags();
                if (mainReq->isCondSwap()) {
                    assert (i == _fault.size());
                    assert(_res);
                    mainReq->setExtraData(*_res);
                }
                if (i == _fault.size()) {
                    _inst->fault = NoFault;
                    setState(State::Request);
                } else {
                  _inst->fault = _fault[i];
                  setState(State::PartialFault);
                }
            } else {
                _inst->fault = _fault[0];
                setState(State::Fault);
            }
        }

    }
}

void
LSQ::SingleDataRequest::initiateTranslation()
{
    assert(_requests.size() == 0);

    addRequest(_addr, _size, _byteEnable);

    if (_requests.size() > 0) {
        _requests.back()->setReqInstSeqNum(_inst->seqNum);
        _requests.back()->taskId(_taskId);
        _inst->translationStarted(true);
        setState(State::Translation);
        flags.set(Flag::TranslationStarted);

        _inst->savedReq = this;
        sendFragmentToTranslation(0);
    } else {
        _inst->setMemAccPredicate(false);
    }
}

PacketPtr
LSQ::SplitDataRequest::mainPacket()
{
    return _mainPacket;
}

RequestPtr
LSQ::SplitDataRequest::mainRequest()
{
    return mainReq;
}

void
LSQ::SplitDataRequest::initiateTranslation()
{
    auto cacheLineSize = _port.cacheLineSize();
    Addr base_addr = _addr;
    Addr next_addr = addrBlockAlign(_addr + cacheLineSize, cacheLineSize);
    Addr final_addr = addrBlockAlign(_addr + _size, cacheLineSize);
    uint32_t size_so_far = 0;

    mainReq = std::make_shared<Request>(base_addr,
                _size, _flags, _inst->requestorId(),
                _inst->instAddr(), _inst->contextId());
    mainReq->setByteEnable(_byteEnable);

    // Paddr is not used in mainReq. However, we will accumulate the flags
    // from the sub requests into mainReq by calling setFlags() in finish().
    // setFlags() assumes that paddr is set so flip the paddr valid bit here to
    // avoid a potential assert in setFlags() when we call it from  finish().
    mainReq->setPaddr(0);

    /* Get the pre-fix, possibly unaligned. */
    auto it_start = _byteEnable.begin();
    auto it_end = _byteEnable.begin() + (next_addr - base_addr);
    addRequest(base_addr, next_addr - base_addr,
                     std::vector<bool>(it_start, it_end));
    size_so_far = next_addr - base_addr;

    /* We are block aligned now, reading whole blocks. */
    base_addr = next_addr;
    while (base_addr != final_addr) {
        auto it_start = _byteEnable.begin() + size_so_far;
        auto it_end = _byteEnable.begin() + size_so_far + cacheLineSize;
        addRequest(base_addr, cacheLineSize,
                         std::vector<bool>(it_start, it_end));
        size_so_far += cacheLineSize;
        base_addr += cacheLineSize;
    }

    /* Deal with the tail. */
    if (size_so_far < _size) {
        auto it_start = _byteEnable.begin() + size_so_far;
        auto it_end = _byteEnable.end();
        addRequest(base_addr, _size - size_so_far,
                         std::vector<bool>(it_start, it_end));
    }

    if (_requests.size() > 0) {
        /* Setup the requests and send them to translation. */
        for (auto& r: _requests) {
            r->setReqInstSeqNum(_inst->seqNum);
            r->taskId(_taskId);
        }

        _inst->translationStarted(true);
        setState(State::Translation);
        flags.set(Flag::TranslationStarted);
        _inst->savedReq = this;
        numInTranslationFragments = 0;
        numTranslatedFragments = 0;
        _fault.resize(_requests.size());

        for (uint32_t i = 0; i < _requests.size(); i++) {
            sendFragmentToTranslation(i);
        }
    } else {
        _inst->setMemAccPredicate(false);
    }
}

LSQ::LSQRequest::LSQRequest(
        LSQUnit *port, const DynInstPtr& inst, bool isLoad) :
    _state(State::NotIssued), _senderState(nullptr),
    _port(*port), _inst(inst), _data(nullptr),
    _res(nullptr), _addr(0), _size(0), _flags(0),
    _numOutstandingPackets(0), _amo_op(nullptr)
{
    flags.set(Flag::IsLoad, isLoad);
    flags.set(Flag::WbStore,
              _inst->isStoreConditional() || _inst->isAtomic());
    flags.set(Flag::IsAtomic, _inst->isAtomic());
    install();
}

LSQ::LSQRequest::LSQRequest(
        LSQUnit *port, const DynInstPtr& inst, bool isLoad,
        const Addr& addr, const uint32_t& size, const Request::Flags& flags_,
           PacketDataPtr data, uint64_t* res, AtomicOpFunctorPtr amo_op)
    : _state(State::NotIssued), _senderState(nullptr),
    numTranslatedFragments(0),
    numInTranslationFragments(0),
    _port(*port), _inst(inst), _data(data),
    _res(res), _addr(addr), _size(size),
    _flags(flags_),
    _numOutstandingPackets(0),
    _amo_op(std::move(amo_op))
{
    flags.set(Flag::IsLoad, isLoad);
    flags.set(Flag::WbStore,
              _inst->isStoreConditional() || _inst->isAtomic());
    flags.set(Flag::IsAtomic, _inst->isAtomic());
    install();
}

void
LSQ::LSQRequest::install()
{
    if (isLoad()) {
        _port.loadQueue[_inst->lqIdx].setRequest(this);
    } else {
        // Store, StoreConditional, and Atomic requests are pushed
        // to this storeQueue
        _port.storeQueue[_inst->sqIdx].setRequest(this);
    }
}

bool LSQ::LSQRequest::squashed() const { return _inst->isSquashed(); }

void
LSQ::LSQRequest::addRequest(Addr addr, unsigned size,
           const std::vector<bool>& byte_enable)
{
    if (isAnyActiveElement(byte_enable.begin(), byte_enable.end())) {
        auto request = std::make_shared<Request>(
                addr, size, _flags, _inst->requestorId(),
                _inst->instAddr(), _inst->contextId(),
                std::move(_amo_op));
        request->setByteEnable(byte_enable);
        _requests.push_back(request);
    }
}

LSQ::LSQRequest::~LSQRequest()
{
    assert(!isAnyOutstandingRequest());
    _inst->savedReq = nullptr;
    if (_senderState)
        delete _senderState;

    for (auto r: _packets)
        delete r;
};

void
LSQ::LSQRequest::sendFragmentToTranslation(int i)
{
    numInTranslationFragments++;
    _port.getMMUPtr()->translateTiming(request(i), _inst->thread->getTC(),
            this, isLoad() ? BaseMMU::Read : BaseMMU::Write);
}

bool
LSQ::SingleDataRequest::recvTimingResp(PacketPtr pkt)
{
    assert(_numOutstandingPackets == 1);
    auto state = dynamic_cast<LSQSenderState*>(pkt->senderState);
    flags.set(Flag::Complete);
    state->outstanding--;
    assert(pkt == _packets.front());
    _port.completeDataAccess(pkt);
    return true;
}

bool
LSQ::SplitDataRequest::recvTimingResp(PacketPtr pkt)
{
    auto state = dynamic_cast<LSQSenderState*>(pkt->senderState);
    uint32_t pktIdx = 0;
    while (pktIdx < _packets.size() && pkt != _packets[pktIdx])
        pktIdx++;
    assert(pktIdx < _packets.size());
    numReceivedPackets++;
    state->outstanding--;
    if (numReceivedPackets == _packets.size()) {
        flags.set(Flag::Complete);
        /* Assemble packets. */
        PacketPtr resp = isLoad()
            ? Packet::createRead(mainReq)
            : Packet::createWrite(mainReq);
        if (isLoad())
            resp->dataStatic(_inst->memData);
        else
            resp->dataStatic(_data);
        resp->senderState = _senderState;
        _port.completeDataAccess(resp);
        delete resp;
    }
    return true;
}

void
LSQ::SingleDataRequest::buildPackets()
{
    assert(_senderState);
    /* Retries do not create new packets. */
    if (_packets.size() == 0) {
        _packets.push_back(
                isLoad()
                    ?  Packet::createRead(request())
                    :  Packet::createWrite(request()));
        _packets.back()->dataStatic(_inst->memData);
        _packets.back()->senderState = _senderState;

        // hardware transactional memory
        // If request originates in a transaction (not necessarily a HtmCmd),
        // then the packet should be marked as such.
        if (_inst->inHtmTransactionalState()) {
            _packets.back()->setHtmTransactional(
                _inst->getHtmTransactionUid());

            DPRINTF(HtmCpu,
              "HTM %s pc=0x%lx - vaddr=0x%lx - paddr=0x%lx - htmUid=%u\n",
              isLoad() ? "LD" : "ST",
              _inst->instAddr(),
              _packets.back()->req->hasVaddr() ?
                  _packets.back()->req->getVaddr() : 0lu,
              _packets.back()->getAddr(),
              _inst->getHtmTransactionUid());
        }
    }
    assert(_packets.size() == 1);
}

void
LSQ::SplitDataRequest::buildPackets()
{
    /* Extra data?? */
    Addr base_address = _addr;

    if (_packets.size() == 0) {
        /* New stuff */
        if (isLoad()) {
            _mainPacket = Packet::createRead(mainReq);
            _mainPacket->dataStatic(_inst->memData);

            // hardware transactional memory
            // If request originates in a transaction,
            // packet should be marked as such
            if (_inst->inHtmTransactionalState()) {
                _mainPacket->setHtmTransactional(
                    _inst->getHtmTransactionUid());
                DPRINTF(HtmCpu,
                  "HTM LD.0 pc=0x%lx-vaddr=0x%lx-paddr=0x%lx-htmUid=%u\n",
                  _inst->instAddr(),
                  _mainPacket->req->hasVaddr() ?
                      _mainPacket->req->getVaddr() : 0lu,
                  _mainPacket->getAddr(),
                  _inst->getHtmTransactionUid());
            }
        }
        for (int i = 0; i < _requests.size() && _fault[i] == NoFault; i++) {
            RequestPtr r = _requests[i];
            PacketPtr pkt = isLoad() ? Packet::createRead(r)
                                     : Packet::createWrite(r);
            ptrdiff_t offset = r->getVaddr() - base_address;
            if (isLoad()) {
                pkt->dataStatic(_inst->memData + offset);
            } else {
                uint8_t* req_data = new uint8_t[r->getSize()];
                std::memcpy(req_data,
                        _inst->memData + offset,
                        r->getSize());
                pkt->dataDynamic(req_data);
            }
            pkt->senderState = _senderState;
            _packets.push_back(pkt);

            // hardware transactional memory
            // If request originates in a transaction,
            // packet should be marked as such
            if (_inst->inHtmTransactionalState()) {
                _packets.back()->setHtmTransactional(
                    _inst->getHtmTransactionUid());
                DPRINTF(HtmCpu,
                  "HTM %s.%d pc=0x%lx-vaddr=0x%lx-paddr=0x%lx-htmUid=%u\n",
                  isLoad() ? "LD" : "ST",
                  i+1,
                  _inst->instAddr(),
                  _packets.back()->req->hasVaddr() ?
                      _packets.back()->req->getVaddr() : 0lu,
                  _packets.back()->getAddr(),
                  _inst->getHtmTransactionUid());
            }
        }
    }
    assert(_packets.size() > 0);
}

void
LSQ::SingleDataRequest::sendPacketToCache()
{
    assert(_numOutstandingPackets == 0);
    if (lsqUnit()->trySendPacket(isLoad(), _packets.at(0)))
        _numOutstandingPackets = 1;
}

void
LSQ::SplitDataRequest::sendPacketToCache()
{
    /* Try to send the packets. */
    while (numReceivedPackets + _numOutstandingPackets < _packets.size() &&
            lsqUnit()->trySendPacket(isLoad(),
                _packets.at(numReceivedPackets + _numOutstandingPackets))) {
        _numOutstandingPackets++;
    }
}

Cycles
LSQ::SingleDataRequest::handleLocalAccess(
        gem5::ThreadContext *thread, PacketPtr pkt)
{
    return pkt->req->localAccessor(thread, pkt);
}

Cycles
LSQ::SplitDataRequest::handleLocalAccess(
        gem5::ThreadContext *thread, PacketPtr mainPkt)
{
    Cycles delay(0);
    unsigned offset = 0;

    for (auto r: _requests) {
        PacketPtr pkt =
            new Packet(r, isLoad() ? MemCmd::ReadReq : MemCmd::WriteReq);
        pkt->dataStatic(mainPkt->getPtr<uint8_t>() + offset);
        Cycles d = r->localAccessor(thread, pkt);
        if (d > delay)
            delay = d;
        offset += r->getSize();
        delete pkt;
    }
    return delay;
}

bool
LSQ::SingleDataRequest::isCacheBlockHit(Addr blockAddr, Addr blockMask)
{
    return ( (LSQRequest::_requests[0]->getPaddr() & blockMask) == blockAddr);
}

/**
 * Caches may probe into the load-store queue to enforce memory ordering
 * guarantees. This method supports probes by providing a mechanism to compare
 * snoop messages with requests tracked by the load-store queue.
 *
 * Consistency models must enforce ordering constraints. TSO, for instance,
 * must prevent memory reorderings except stores which are reordered after
 * loads. The reordering restrictions negatively impact performance by
 * cutting down on memory level parallelism. However, the core can regain
 * performance by generating speculative loads. Speculative loads may issue
 * without affecting correctness if precautions are taken to handle invalid
 * memory orders. The load queue must squash under memory model violations.
 * Memory model violations may occur when block ownership is granted to
 * another core or the block cannot be accurately monitored by the load queue.
 */
bool
LSQ::SplitDataRequest::isCacheBlockHit(Addr blockAddr, Addr blockMask)
{
    bool is_hit = false;
    for (auto &r: _requests) {
       /**
        * The load-store queue handles partial faults which complicates this
        * method. Physical addresses must be compared between requests and
        * snoops. Some requests will not have a valid physical address, since
        * partial faults may have outstanding translations. Therefore, the
        * existence of a valid request address must be checked before
        * comparing block hits. We assume no pipeline squash is needed if a
        * valid request address does not exist.
        */
        if (r->hasPaddr() && (r->getPaddr() & blockMask) == blockAddr) {
            is_hit = true;
            break;
        }
    }
    return is_hit;
}

bool
LSQ::DcachePort::recvTimingResp(PacketPtr pkt)
{
    return lsq->recvTimingResp(pkt);
}

void
LSQ::DcachePort::recvTimingSnoopReq(PacketPtr pkt)
{
    for (ThreadID tid = 0; tid < cpu->numThreads; tid++) {
        if (cpu->getCpuAddrMonitor(tid)->doMonitor(pkt)) {
            cpu->wakeup(tid);
        }
    }
    lsq->recvTimingSnoopReq(pkt);
}

void
LSQ::DcachePort::recvReqRetry()
{
    lsq->recvReqRetry();
}

LSQ::HtmCmdRequest::HtmCmdRequest(LSQUnit* port, const DynInstPtr& inst,
        const Request::Flags& flags_) :
    SingleDataRequest(port, inst, true, 0x0lu, 8, flags_,
        nullptr, nullptr, nullptr)
{
    assert(_requests.size() == 0);

    addRequest(_addr, _size, _byteEnable);

    if (_requests.size() > 0) {
        _requests.back()->setReqInstSeqNum(_inst->seqNum);
        _requests.back()->taskId(_taskId);
        _requests.back()->setPaddr(_addr);
        _requests.back()->setInstCount(_inst->getCpuPtr()->totalInsts());

        _inst->strictlyOrdered(_requests.back()->isStrictlyOrdered());
        _inst->fault = NoFault;
        _inst->physEffAddr = _requests.back()->getPaddr();
        _inst->memReqFlags = _requests.back()->getFlags();
        _inst->savedReq = this;

        setState(State::Translation);
    } else {
        panic("unexpected behaviour");
    }
}

void
LSQ::HtmCmdRequest::initiateTranslation()
{
    // Transaction commands are implemented as loads to avoid significant
    // changes to the cpu and memory interfaces
    // The virtual and physical address uses a dummy value of 0x00
    // Address translation does not really occur thus the code below

    flags.set(Flag::TranslationStarted);
    flags.set(Flag::TranslationFinished);

    _inst->translationStarted(true);
    _inst->translationCompleted(true);

    setState(State::Request);
}

void
LSQ::HtmCmdRequest::finish(const Fault &fault, const RequestPtr &req,
        gem5::ThreadContext* tc, BaseMMU::Mode mode)
{
    panic("unexpected behaviour");
}

Fault
LSQ::read(LSQRequest* req, int load_idx)
{
    ThreadID tid = cpu->contextToThread(req->request()->contextId());

    return thread.at(tid).read(req, load_idx);
}

Fault
LSQ::write(LSQRequest* req, uint8_t *data, int store_idx)
{
    ThreadID tid = cpu->contextToThread(req->request()->contextId());

    return thread.at(tid).write(req, data, store_idx);
}

} // namespace o3
} // namespace gem5
