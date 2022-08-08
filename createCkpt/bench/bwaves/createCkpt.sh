#!/bin/bash

bench=bwaves.riscv
settings="settings_pk"
options=""
input=""

#riscv-pk stack = 3fbe9000, mmap = 3fbe9000
#fpga: 0x3f0000000 0x3d0000000

gem5_dir=/home/gem5_src/gem5
echo "$gem5_dirbuild/RISCV/gem5.fast ./configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=$settings -c $bench $options"
$gem5_dir/build/RISCV/gem5.fast $gem5_dir/configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=$settings -c $bench --options="$options" --input="$input"
