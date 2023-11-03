1. 更改riscv integer寄存器的数量，用作于临时寄存器使用
    - 修改的文件：arch/riscv/regs/int.hh
    ```c
        const int NumIntArchRegs = 32;

        const int NumIntArchRegs = 32+4;
    ```

2. 更改riscv decoder的算法，使其单独支持扩展的指令
    - 修改的文件：arch/riscv/decoder.cc
    ```c
        //before
        Decoder::decode(ExtMachInst mach_inst, Addr addr)
        {
            DPRINTF(Decode, "Decoding instruction 0x%08x at address %#x\n",
                    mach_inst, addr);

            StaticInstPtr &si = instMap[mach_inst];
            if (!si)
                si = decodeInst(mach_inst);

            si->isCompressed = compressed(mach_inst);

            DPRINTF(Decode, "Decode: Decoded %s instruction: %#x\n",
                    si->getName(), mach_inst);
            return si;
        }


        //modified
        Decoder::decode(ExtMachInst mach_inst, Addr addr)
        {
            DPRINTF(Decode, "Decoding instruction 0x%08x at address %#x\n",
                    mach_inst, addr);

            //new imm[11:0] rs1 000 rd 0010011
            //addi x0, rs1, #9+: write temp,     op = 0x13, func3 = 0, 
            //addi x0, rs1, #36+: read temp,   op = 0x13, func3 = 0, 
            //addi x0, rs1, #64: jmp rtemp,    op = 0x13, func3 = 0, 

            unsigned int taddinst = 0x80b3; //add rd (1), rs1(1), rs2(0)
            unsigned int tjalrinst = 0x8067; //jalr rd(0), rs1(1), 0
            ExtMachInst old_mach_inst = mach_inst;

            unsigned int opcode = 0, rs1 = 0, rs2 = 0, rd = 1, imm12 = 0, func3 = 0;
            bool isRTemp = false, isWTemp = false, isJTemp = false;
            if (!compressed(emi)) {
                opcode = mach_inst % 128;
                imm12 = (mach_inst >> 20);
                func3 = (mach_inst >> 12) % 8;
                rs1 = (mach_inst >> 15) % 32;
                rs2 = (mach_inst >> 20) % 4;
                rd = (mach_inst >> 7) % 32;

                bool isNewInst = (opcode == 0x13) && (func3 == 0) && (rd == 0);

                isWTemp = isNewInst && (imm12 >= 9 && imm12 <= 12);
                isRTemp = isNewInst && (imm12 >= 36 && imm12 <= 39);
                isJTemp = isNewInst && (imm12 == 64);
            }
            
            if(isRTemp || isWTemp){
                mach_inst = taddinst;
            }
            else if(isJTemp){
                mach_inst = tjalrinst;
            }

            StaticInstPtr &si = instMap[mach_inst];
            if (!si || isRTemp || isWTemp || isJTemp){ //如果是临时指令，则不从cache中取，而是直接重新译码
                si = decodeInst(mach_inst);
            }  

            if(isRTemp) {
                si->setSrcRegIdx(0, RegId(IntRegClass, 32+imm12-36));
                si->setDestRegIdx(0, RegId(IntRegClass, rs1));
            }
            else if(isWTemp) {
                si->setSrcRegIdx(0, RegId(IntRegClass, rs1));
                si->setDestRegIdx(0, RegId(IntRegClass, 32+imm12-9));
            }
            else if(isJTemp) {
                si->setSrcRegIdx(0, RegId(IntRegClass, 32+rs1%4));
            }

            DPRINTF(Decode, "Decode: Decoded %s instruction: %#x\n",
                    si->getName(), mach_inst);
            return si;
        }

    ```