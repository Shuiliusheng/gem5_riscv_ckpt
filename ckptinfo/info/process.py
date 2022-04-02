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
        print("\t params: ", self.params)
        print("\t data: ", self.data)

    def show1(self):
        print("sysinfo: " + self.pc, self.num, self.name, self.params[0], self.params[1], self.params[2], self.hasret, self.ret, self.buffaddr, len(self.data))
        print("\t data: ", self.data)

class StartPoint:
    inst_num = ""
    pc = " "
    npc = " "
    intdata = []
    fpdata = []
    def __init__(self, inst_num = "0", pc = " ", npc = " ", intdata = [], fpdata = []):
        self.inst_num = inst_num
        self.pc = pc
        self.npc = npc
        self.intdata = intdata
        self.fpdata = fpdata

    def show(self):
        print("startpoint, instplace: " + self.inst_num + ", pc: " + self.pc + ", npc: " + self.npc)
        # idx = 0
        # while idx < len(self.intdata):
        #     print("int reg ", idx, ": " , int(self.intdata[idx], 16))
        #     idx = idx + 1

        # idx = 0
        # while idx < len(self.fpdata):
        #     print("fp reg ", idx, ": " , int(self.fpdata[idx], 16))
        #     idx = idx + 1
    
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


def processRegInfo(reginfo):
    intreg = json.loads(reginfo[0])
    fpreg = json.loads(reginfo[1])
    intdata = intreg['data']
    fpdata = fpreg['data']
    startpoint = StartPoint(intreg['inst_num'], intreg['inst_pc'], intreg['npc'], intdata, fpdata)
    return startpoint
    

def processSyscall(syscallinfo):
    order = 0
    idx = 0
    syscallinfos = []
    while idx < len(syscallinfo):
        enter = json.loads(syscallinfo[idx])
        print(enter)
        if enter['type'] != "syscall enter":
            print("something is wrong enter!")
            print(enter)
            break
        line1 = json.loads(syscallinfo[idx+1])
        if line1['type'] == "syscall info":
            line2 = json.loads(syscallinfo[idx+2])
            if line2['type'] != "syscall return":
                print("something is wrong return!")
                print(line2)
                break
            data = []
            if line1['info'] == "write" or line1['info'] == "read":
                size = int(line2['val'], 16)
                if size != -1:
                    data = line1['data'][:size]
            elif line1['info'] == "setdata":
                data = line1['data']
            
            hasret = line2['res'] == "has ret"
            print(line1)
            bufaddr = "0x0"
            if ( 'buf' in line1 ): 
                bufaddr = line1['buf']
            sinfo = SyscallInfo(enter['pc'], enter['sysnum'], line2['sysname'], enter['param'], hasret, line2['val'], data, bufaddr)
            syscallinfos.append(sinfo)
            idx = idx + 2

        else:
            if line1['type'] != "syscall return":
                print("something is wrong return 1 !")
                print(line1)
                break
            hasret = line1['res'] == "has ret"
            sinfo = SyscallInfo(enter['pc'], enter['sysnum'], line1['sysname'], enter['param'], hasret, line1['val'], [], "0x0")
            syscallinfos.append(sinfo)
            idx = idx + 1

        idx = idx + 1
    return syscallinfos


def depart_data(value, size):
    idx = 0
    res = []
    val = int(value, 16)
    while idx < size:
        res.append(val % 256)
        val  = val >> 8
        idx = idx + 1
    return res

#combine the load with byte access size
def combine(loadinfo):
    idx = 0
    infos = []
    while idx < len(loadinfo):
        saddr = loadinfo[idx].addr
        step = 0
        if idx + 7 < len(loadinfo) and loadinfo[idx+7].addr == saddr + 7:
            step = 8
        elif idx + 3 < len(loadinfo) and loadinfo[idx+3].addr == saddr + 3:
            step = 4
        elif idx + 1 < len(loadinfo) and loadinfo[idx+1].addr == saddr + 1:
            step = 2
        else:
            step = 0
        
        if step != 0:
            idx1 = 0
            data = 0
            while idx1 < step:
                data = data + (loadinfo[idx+idx1].data << (idx1*8))
                idx1 = idx1 + 1
            infos.append(Memdata(loadinfo[idx].addr, data, step))
            idx = idx + step
        else:
            infos.append(loadinfo[idx])
            idx = idx + 1

    return infos

# get the first load information
def getLoadInfo(meminfo):
    loadinfo = []
    preload = set()
    prestore = set()
    idx = 0
    while idx < len(meminfo):
        line = json.loads(meminfo[idx])
        if line['type'] == "mem_read" or line['type'] == "mem_atomic":
            saddr = int(line['addr'], 16)
            size = int(line['size'], 16)
            data = depart_data(line['data'], size)
            idx1 = 0

            while idx1 < size:
                val = saddr + idx1
                if (val not in prestore) and (val not in preload):
                    preload.add(val)
                    loadinfo.append(Memdata(val, data[idx1], 1))
                idx1 = idx1 + 1
        else:
            saddr = int(line['addr'], 16)
            size = int(line['size'], 16)
            idx1 = 0
            while idx1 < size:
                val = saddr + idx1
                if val not in prestore:
                    prestore.add(val)
                idx1 = idx1 + 1
        idx = idx + 1
    
    loadinfo.sort(key = getaddr)
    loadinfo = combine(loadinfo)
    return loadinfo

# get the load store access range in memory with 4KB step
def getMemRange(meminfo):
    infos = []
    res = []
    addrset = set()
    idx = 0
    while idx < len(meminfo):
        line = json.loads(meminfo[idx])
        saddr = int(line['addr'], 16)
        size = int(line['size'], 16)
        idx1 = 0
        while idx1 < size:
            val = saddr + idx1
            if val not in addrset:
                addrset.add(val)
                infos.append(val)
            idx1 = idx1 + 1
        idx = idx + 1
    
    idx = 0
    infos.sort()
    while idx < len(infos):
        saddr = (infos[idx] >> 12) << 12
        eaddr = saddr + 4096

        idx1 = idx + 1
        while idx1 < len(infos):
            if infos[idx1] >= eaddr:
                break
            idx1 = idx1 + 1
        res.append(Memdata(saddr, 0, 4096))
        idx = idx1

    idx = 0
    memrange = []
    while idx < len(res):
        saddr = res[idx].addr
        idx1 = idx + 1
        while idx1 < len(res):
            if res[idx1].addr - saddr != (idx1-idx)*4096:
                break
            idx1 = idx1 + 1
        memrange.append(Memdata(saddr, 0, (idx1-idx)*4096))
        idx = idx1
    return memrange

def process(end, reginfo, meminfo, syscallinfo):
    startpoint = processRegInfo(reginfo)
    syscallinfos = processSyscall(syscallinfo)
    loadinfo = getLoadInfo(meminfo)
    memrange = getMemRange(meminfo)
    writefile(startpoint, syscallinfos, loadinfo, memrange, end)
    startpoint.show()
    for syscall in syscallinfos:
        syscall.show()
    for mem in memrange:
        mem.show()
    os.system("pause")
    

def readfile(filename):
    file = open(filename) 
    meminfo = []
    reginfo = []
    syscallinfo = []
    end = 0
    numline = 0
    while 1:
        line = file.readline()
        numline += 1
        if not line:
            break
        if line.find("{") == -1 and line.find("}") == -1:
            continue

        if line.find("{") == -1 or line.find("}") == -1:
            print(numline, line)
            continue
        
        if line.find("mem_read") != -1 or line.find("mem_write") != -1 or line.find("mem_atomic") != -1:
            meminfo.append(line)
            continue

        if line.find("syscall") != -1:
            syscallinfo.append(line)
            continue

        if line.find("fp_regs") != -1:
            reginfo.append(line)
            continue

        if line.find("int_regs") != -1:
            if len(reginfo) > 0:
                end = json.loads(line)['inst_num']
                process(end, reginfo, meminfo, syscallinfo)
            reginfo = []
            meminfo = []
            syscallinfo = []
            reginfo.append(line)
    file.close()

def writefile(startpoint, syscallinfos, loadinfo, memrange, end):
    ckptname = "ckpt_"+str(startpoint.inst_num)+"_"+str(end)+".info"
    syscallname = "ckpt_syscall_"+str(startpoint.inst_num)+"_"+str(end)+".info"

    f1 = open(ckptname, 'wb')
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