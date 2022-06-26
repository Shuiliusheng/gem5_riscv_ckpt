#!/bin/bash

if [[ $# < 2 ]]; then
    echo "parameters are not enough!"
    echo "./run.sh bench settings bench_option"
    exit
fi

bench=$1
if [[ ! -e $bench ]]; then
    echo $bench " is not exist!"
    exit
fi 

settings=$2
options="--options=$3"

#riscv-pk stack = 3fbe9000, mmap = 3fbe9000
#stacktop=801017856  #0x2fbe9000
#mmapend=789483520   #0x2f0e9000

#stacktop=10000 #270582939648  #0x3f00000000
#mmapend=10000 #261993005056   #0x3d00000000

gem5_dir=/home/gem5_src/gem5
echo "build/RISCV/gem5.fast ./configs/example/se.py --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=$settings -c $bench $options"
$gem5_dir/build/RISCV/gem5.fast $gem5_dir/configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=$settings -c $bench "$options"
