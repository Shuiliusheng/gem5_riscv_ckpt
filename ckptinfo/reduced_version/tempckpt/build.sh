#!/bin/bash

cc=gcc
flags="-static -O2"
dflags=""
target="showckpt"
if [[ $# > 0 ]]; then
    dflags="-DSHOWLOG"
fi

filelist="ckpt_load_rdc.cpp elf_load.cpp readckpt.cpp"
echo ${cc} $filelist $flags $dflags -o ${target}
${cc} $filelist $flags $dflags -o ${target}

