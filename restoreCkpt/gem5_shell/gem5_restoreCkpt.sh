#!/bin/bash

if [[ $# < 2 ]]; then
    echo "parameters are not enough!"
    echo $1 $2
    exit
fi

#info directory
infodir="ckptinfo/info"
readckpt="ckptinfo/readckpt/readckpt.riscv"

bench=$1
simInsts=$2

basename=`echo $(basename $bench) |awk -F '.' '{print $1}'`
ckptinfo=$infodir/${basename}_ckpt_${simInsts}.info
ckptsysinfo=$infodir/${basename}_ckpt_syscall_${simInsts}.info

option="--options=$ckptinfo $ckptsysinfo $bench"

debugflags="" #--debug-flag=ShowDetail

if [[ ! -e $bench ]]; then
    echo $bench " is not exist!"
    exit
fi 

if [[ ! -e $ckptinfo ]]; then
    echo $ckptinfo " is not exist!"
    exit
fi 

if [[ ! -e $ckptsysinfo ]]; then
    echo $ckptsysinfo " is not exist!"
    exit
fi 

if [[ ! -e $readckpt ]]; then
    echo $readckpt " is not exist!"
    exit
fi 

echo "build/RISCV/gem5.opt $debugflags ./configs/example/se.py -c $readckpt "$option""
build/RISCV/gem5.opt $debugflags ./configs/example/se.py -c $readckpt "$option"
