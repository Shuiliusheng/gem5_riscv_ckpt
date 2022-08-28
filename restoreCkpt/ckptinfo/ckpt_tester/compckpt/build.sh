#!/bin/bash

cc=g++
flags="-static -O2"
dflags=""
target="testckpt"

filelist="*.cpp"
echo ${cc} $filelist $flags $dflags -o ${target}
${cc} $filelist $flags $dflags -w -o ${target}

