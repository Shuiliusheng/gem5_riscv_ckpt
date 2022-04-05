#!/bin/bash

if [[ $# < 2 ]]; then
    echo "parameters are not enough!"
    echo $1 $2
    exit
fi

#info directory
exedir="/home/cuihongwei/experiment/gem5_dir/gem5_riscv_ckpt/ckptinfo/test"
infodir="/home/cuihongwei/experiment/gem5_dir/gem5_riscv_ckpt/ckptinfo/info"
readckpt="/home/cuihongwei/experiment/gem5_dir/gem5_riscv_ckpt/ckptinfo/readckpt/readckpt.riscv"
pk=/home/cuihongwei/unicore-boom-fpga/riscv-pk/build/pk

bench=$exedir/$1
simInsts=$2

basename=`echo $(basename $bench) |awk -F '.' '{print $1}'`
ckptinfo=$infodir/${basename}_ckpt_${simInsts}.info
ckptsysinfo=$infodir/${basename}_ckpt_syscall_${simInsts}.info

options="$ckptinfo $ckptsysinfo $bench"

debug="" #--debug-flag=ShowDetail

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

if [[ ! -e $pk ]]; then
	echo $pk " is not exist."
	exit
fi

echo "./build/simulator-MediumBoomConfig $debug $pk $readckpt $options"
./build/simulator-MediumBoomConfig $debug $pk $readckpt $options
