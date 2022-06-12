#!/usr/bin/env python3

import os
import json
import sys
import struct

class SyscallInfo:
    pc = ""
    num = ""
    name = ""
    params = []
    hasret = False
    ret = ""
    data = []
    bufaddr = ""
    def __init__(self, pc = "", num = "", name = "", params = [], hasret=False, ret="", data = [], bufaddr = ""):
        self.pc = pc
        self.num = num
        self.name = name
        self.params = params
        self.hasret = hasret
        self.ret = ret
        self.data = data
        self.bufaddr = bufaddr

    def show(self):
        print("sysinfo: " + self.num + " (" + self.name + "), pc: " + self.pc + ", ret: " + self.ret + ", bufaddr: " + self.bufaddr + ", data: " , len(self.data))
        # print("\t params: ", self.params)
        # print("\t data: ", self.data)

    def show1(self):
        print("sysinfo: " + self.pc, self.num, self.name, self.params[0], self.params[1], self.params[2], self.hasret, self.ret, self.bufaddr, len(self.data))
        # print("\t data: ", self.data)

class StartPoint:
    inst_num = ""
    simnum = ""
    length = ""
    pc = " "
    npc = " "
    exitpc = " "
    intdata = []
    fpdata = []
    def __init__(self, inst_num = "0", pc = " ", npc = " ", intdata = [], fpdata = [], simnum = "", length="", exitpc=""):
        self.inst_num = inst_num
        self.pc = pc
        self.npc = npc
        self.intdata = intdata
        self.fpdata = fpdata
        self.simnum = simnum
        self.length = length
        self.exitpc = exitpc

    def show(self):
        print("startpoint, start: " + self.inst_num + ", pc: " + self.pc + ", simnum: " + self.simnum)
    
class Memdata:
    addr = 0
    data = 0
    size = 0
    def __init__(self, addr = 0, data = 0, size = 0):
        self.addr = addr
        self.data = data
        self.size = size

    def show(self):
        print('addr: 0x%x'%self.addr, ", data: 0x%x"%self.data, ", size: 0x%x"%self.size)

def getaddr(load):
    return load.addr

def toBytes(val, size):
    bs = val.to_bytes(length=size, byteorder='little', signed=False)
    return bs

def readfile(filename):
    file1 = open(filename) 
    memrange = []
    textrange = []
    reginfo = []
    syscallinfo = []
    loadinfo = []
    numline = 0
    while 1:
        line = file1.readline()
        numline += 1
        if not line:
            break
        if line.find("{") == -1 and line.find("}") == -1:
            continue

        if line.find("{") == -1 or line.find("}") == -1:
            print(numline, line)
            continue

        line = "{" + line.split("{")[1].split("}")[0] + "}"
        words = json.loads(line)
        
        if words['type'] == "fld":
            loadinfo.append(Memdata(int(words['a'], 16), int(words['d'], 16), int(words['s'], 16)))
            continue

        if words['type'].find("int_regs") != -1:
            reginfo.append(words)
            continue

        if words['type'].find("fp_regs") != -1:
            reginfo.append(words)
            continue

        if words['type'].find("textRange") != -1:
            idx = 0 
            while idx < len(words['addr']):
                textrange.append(Memdata(int(words['addr'][idx], 16), 0, int(words['size'][idx], 16)))
                idx = idx + 1
            continue

        if words['type'].find("memRange") != -1:
            idx = 0 
            while idx < len(words['addr']):
                memrange.append(Memdata(int(words['addr'][idx], 16), 0, int(words['size'][idx], 16)))
                idx = idx + 1
            continue

        if words['type'] == "syscall":
            sinfo = SyscallInfo(words['pc'], words['sysnum'], words['sysname'], words['params'], words['hasret'] == "1", words['ret'], words['data'], words['bufaddr'])
            syscallinfo.append(sinfo)
            continue

        if words['type'].find("ckptinfo") != -1:
            if len(reginfo) > 0:
                startpoint.intdata = reginfo[0]['data']
                startpoint.fpdata = reginfo[1]['data']
                writefile(startpoint, syscallinfo, loadinfo, memrange, textrange)

            startpoint = StartPoint(words['startnum'], words['pc'], words['npc'], [], [], words['exitnum'], words['length'], words['exitpc'])
            syscallinfo = []
            loadinfo = []
            memrange = []
            textrange = []
            reginfo = []

    if len(reginfo) > 0:
        startpoint.intdata = reginfo[0]['data']
        startpoint.fpdata = reginfo[1]['data']
        writefile(startpoint, syscallinfo, loadinfo, memrange, textrange)
        
            
    file1.close()

def writefile(startpoint, syscallinfos, loadinfo, memrange, textinfo):
    bench = os.path.splitext(sys.argv[1])[0]
    ckptname = bench+"_ckpt_"+str(startpoint.inst_num)+".info"
    syscallname = bench+"_ckpt_syscall_"+str(startpoint.inst_num)+".info"

    f1 = open(ckptname, 'wb')

    #write textinfo
    f1.write(toBytes(len(textinfo), 8))    ## write number
    for r in textinfo:
        f1.write(toBytes(r.addr, 8))
        f1.write(toBytes(r.size, 8))

    #write sim information
    f1.write(toBytes(int(startpoint.inst_num, 10), 8))
    f1.write(toBytes(int(startpoint.simnum, 10), 8))
    f1.write(toBytes(int(startpoint.exitpc, 16), 8))
    f1.write(toBytes(2, 8))  #exitInst

    #write npc
    f1.write(toBytes(int(startpoint.npc, 16), 8))
    #write int regs
    for reg in startpoint.intdata:
        f1.write(toBytes(int(reg,16), 8))
    for reg in startpoint.fpdata:
        f1.write(toBytes(int(reg,16), 8))
    
    #write memory range
    f1.write(toBytes(len(memrange), 8))    ## write number
    for r in memrange:
        f1.write(toBytes(r.addr, 8))
        f1.write(toBytes(r.size, 8))

    #write load information
    f1.write(toBytes(len(loadinfo), 8))    ## write number
    for load in loadinfo:
        f1.write(toBytes(load.addr, 8))
        f1.write(toBytes(load.size, 8))
        f1.write(toBytes(load.data, 8))

    f1.close()

    data_addr = 10*8*len(syscallinfos) + 8
    invalid_addr = 0xffffffff
    f2 = open(syscallname, 'wb')
    #write syscall information
    print("syscallnum: ", len(syscallinfos))
    f2.write(toBytes(len(syscallinfos), 8))    ## write number
    for syscall in syscallinfos:
        f2.write(toBytes(int(syscall.pc,16), 8))
        f2.write(toBytes(int(syscall.num,16), 8))
        f2.write(toBytes(int(syscall.params[0],16), 8))
        f2.write(toBytes(int(syscall.params[1],16), 8))
        f2.write(toBytes(int(syscall.params[2],16), 8))
        if syscall.hasret :
            f2.write(toBytes(1, 8))
        else:
            f2.write(toBytes(0, 8))
        f2.write(toBytes(int(syscall.ret,16), 8))
        f2.write(toBytes(int(syscall.bufaddr,16), 8))
        data_size = len(syscall.data)
        if data_size == 0:
            f2.write(toBytes(invalid_addr, 8))
        else:
            f2.write(toBytes(data_addr, 8))
        f2.write(toBytes(data_size, 8))
        data_addr = data_addr + data_size
        
    for syscall in syscallinfos:
        data_size = len(syscall.data)
        if data_size == 0:
            continue
        for data in syscall.data:
            s = f2.write(toBytes(int(data, 16), 1))

    f2.close()

    print("----------------------------------------------------")

#----------------------------------------------------------------------------------#
if len(sys.argv) < 2: 
    print("parameters are not enough.\n ./process.py log1")
    exit()

if not os.path.exists(sys.argv[1]):
    print(sys.argv[1]+" is not exist.")
    exit()

readfile(sys.argv[1])