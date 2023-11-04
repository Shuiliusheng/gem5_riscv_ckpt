#!/bin/bash

cc=riscv64-unknown-linux-gnu-g++
objdump=riscv64-unknown-linux-gnu-objdump
flags="-static -O2"
dflags=""
target="readckpt.riscv"

filelist=`find . -name "*.cpp" `
echo ${cc} $filelist $flags $dflags -o ${target}
# ${cc} $filelist $flags $dflags -T ./link.lds -o ${target}
filelist="./takeover_syscall.cpp ./recovery_fload.cpp ./fastlz.cpp ./ckpt_load.cpp ./elf_load.cpp ./readckpt.cpp"
${cc} $filelist $flags $dflags -o ${target}

echo ${objdump} -d ${target} 
${objdump} -d ${target} >read.s

readelf -s readckpt.riscv |grep global_pointer
readelf -s readckpt.riscv |grep readckpt_regs

gp=`readelf -s readckpt.riscv |grep global_pointer|awk -F ' ' '{print $2}'`
temp=`readelf -s readckpt.riscv |grep readckpt_regs|awk -F ' ' '{print $2}'`

dis=$((16#$temp - 16#$gp))
printf %x $dis 
echo