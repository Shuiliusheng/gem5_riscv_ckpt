#!/bin/bash

if [[ $# < 2 ]]; then
    echo "parameters are not enough!"
    echo "./run.sh settingfile bench ckptfile"
    exit
fi


gem5_dir=/home/gem5_src/gem5
readckpt=/home/gem5_src/gem5/local_dir/readckpt_new.riscv
bench=$2
settingfile=$1
ckptfile=$3
options="$ckptfile $bench"

#riscv-pk stack = 3fbe9000, mmap = 3fbe9000
#fpga: 0x3f0000000 0x3d0000000

if [[ ! -e $bench ]]; then
    echo $bench "is not exist!"
    exit
fi

#minpc = 0x200000
#此时正好为readckpt和program之间的pc区分

$gem5_dir/build/RISCV/gem5.fast $gem5_dir/configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB --tracesetting=$settingfile -c $readckpt --options="$options"
