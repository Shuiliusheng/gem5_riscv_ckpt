/*
 * Copyright (c) 2008 The Regents of The University of Michigan
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

#include "dev/x86/i8237.hh"

#include "base/cprintf.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

namespace gem5
{

namespace X86ISA
{

namespace
{

I8237::Register::ReadFunc
readUnimpl(const std::string &label)
{
    return [label](I8237::Register &reg) -> uint8_t {
        panic("Read from i8237 %s unimplemented.", label);
    };
}

I8237::Register::WriteFunc
writeUnimpl(const std::string &label)
{
    return [label](I8237::Register &reg, const uint8_t &value) {
        panic("Write to i8237 %s unimplemented.", label);
    };
}

} // anonymous namespace

I8237::Channel::ChannelAddrReg::ChannelAddrReg(Channel &channel) :
    Register(csprintf("channel %d current address", channel.number))
{
    reader(readUnimpl(name()));
    writer(writeUnimpl(name()));
}

I8237::Channel::ChannelRemainingReg::ChannelRemainingReg(Channel &channel) :
    Register(csprintf("channel %d remaining word count", channel.number))
{
    reader(readUnimpl(name()));
    writer(writeUnimpl(name()));
}

I8237::WriteOnlyReg::WriteOnlyReg(const std::string &new_name, Addr offset) :
    Register(new_name)
{
    reader([offset](I8237::Register &reg) -> uint8_t {
        panic("Illegal read from i8237 register %d.", offset);
    });
}

I8237::I8237(const Params &p) : BasicPioDevice(p, 16), latency(p.pio_latency),
    regs("registers", pioAddr), channels{{{0}, {1}, {2}, {3}}},
    statusCommandReg("status/command"),
    requestReg("request", 0x9),
    setMaskBitReg("set mask bit", 0xa),
    modeReg("mode", 0xb),
    clearFlipFlopReg("clear flip-flop", 0xc),
    temporaryMasterClearReg("temporary/maskter clear"),
    clearMaskReg("clear mask", 0xe),
    writeMaskReg("write mask", 0xf)
{
    // Add the channel address and remaining registers.
    for (auto &channel: channels)
        regs.addRegisters({ channel.addrReg, channel.remainingReg });

    // Add the other registers individually.
    regs.addRegisters({
        statusCommandReg.
            reader(readUnimpl("status register")).
            writer(writeUnimpl("command register")),

        requestReg.
            writer(writeUnimpl("request register")),

        setMaskBitReg.
            writer(this, &I8237::setMaskBit),

        modeReg.
            writer(writeUnimpl("mode register")),

        clearFlipFlopReg.
            writer(writeUnimpl("clear LSB/MSB flip-flop register")),

        temporaryMasterClearReg.
            reader(readUnimpl("temporary register")).
            writer(writeUnimpl("master clear register")),

        clearMaskReg.
            writer(writeUnimpl("clear mask register")),

        writeMaskReg.
            writer(writeUnimpl("write all mask register bits"))
    });
}

void
I8237::setMaskBit(Register &reg, const uint8_t &command)
{
    uint8_t select = bits(command, 1, 0);
    uint8_t bitVal = bits(command, 2);
    if (!bitVal)
        panic("Turning on i8237 channels unimplemented.");
    replaceBits(maskReg, select, bitVal);
}

Tick
I8237::read(PacketPtr pkt)
{
    regs.read(pkt->getAddr(), pkt->getPtr<void>(), pkt->getSize());
    pkt->makeAtomicResponse();
    return latency;
}

Tick
I8237::write(PacketPtr pkt)
{
    regs.write(pkt->getAddr(), pkt->getPtr<void>(), pkt->getSize());
    pkt->makeAtomicResponse();
    return latency;
}

void
I8237::serialize(CheckpointOut &cp) const
{
    SERIALIZE_SCALAR(maskReg);
}

void
I8237::unserialize(CheckpointIn &cp)
{
    UNSERIALIZE_SCALAR(maskReg);
}

} // namespace X86ISA
} // namespace gem5
