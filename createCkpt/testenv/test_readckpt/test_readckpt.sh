#!/bin/bash

gem5dir=/home/gem5_src/gem5
readckpt=/home/gem5_src/gem5/testenv/test_readckpt/readckpt/readckpt.riscv
ckptfile=/home/gem5_src/gem5/testenv/test_readckpt/gcc_nckpt_15000000000_len_6000000_warmup_1000000.info
benchfile=/home/gem5_src/gem5/testenv/test_readckpt/gcc.riscv

option="--options=$ckptfile $benchfile"
debugflags="--debug-flag=Decode"

# use atomic cpu to run readckpt
$gem5dir/build/RISCV/gem5.fast $gem5dir/configs/example/se.py -c $readckpt "$option"

# use o3cpu to run readckpt
$gem5dir/build/RISCV/gem5.fast $gem5dir/configs/example/se.py --cpu-type=DerivO3CPU --caches --mem-type=DDR4_2400_8x8 --mem-size=8GB -c $readckpt "$option"
