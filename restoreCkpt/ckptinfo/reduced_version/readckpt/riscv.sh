#!/bin/bash

cc=riscv64-unknown-linux-gnu-gcc
objdump=riscv64-unknown-linux-gnu-objdump
flags="-static -O2"
dflags=""
target="readckpt_reduced.riscv"
if [[ $# > 0 ]]; then
    dflags="-DSHOWLOG"
fi

filelist="takeover_syscall.cpp ckpt_load_rdc.cpp elf_load.cpp readckpt.cpp"
echo ${cc} $filelist $flags $dflags -o ${target}
${cc} $filelist $flags $dflags -o ${target}

echo ${objdump} -d ${target} 
${objdump} -d ${target} >read.s
