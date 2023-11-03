#!/bin/bash

cc=riscv64-unknown-linux-gnu-g++
objdump=riscv64-unknown-linux-gnu-objdump
flags="-static -O2"
dflags=""
target="readckpt_new.riscv"

filelist=`find . -name "*.cpp" `
echo ${cc} $filelist $flags $dflags -T ./link.lds -o ${target}
filelist="./takeover_syscall.cpp ./recovery_fload.cpp ./fastlz.cpp ./user_jmp.cpp ./elf_load.cpp ./ckpt_load.cpp ./readckpt.cpp"
${cc} $filelist $flags $dflags -T ./link.lds -o ${target}
# ${cc} $filelist $flags $dflags -o ${target}

echo ${objdump} -d ${target} 
${objdump} -d ${target} >read.s
