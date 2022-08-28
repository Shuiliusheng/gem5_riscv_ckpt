#!/bin/bash

cc=g++
flags="-static -O2"
dflags=""
target="compckpt"

filelist="*.cpp"
echo ${cc} $filelist $flags $dflags -o ${target}
${cc} $filelist $flags $dflags -w -o ${target}

