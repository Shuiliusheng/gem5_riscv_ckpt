#!/bin/bash

gem5dir=/home/gem5_src/gem5
readckpt=/home/gem5_src/gem5/testenv/ckpt2trace/readckpt_new.riscv
ckptfile=/home/gem5_src/gem5/testenv/ckpt2trace/gcc_nckpt_15000000000_len_6000000_warmup_1000000.info
benchfile=/home/gem5_src/gem5/testenv/ckpt2trace/gcc.riscv
option="--options=$ckptfile $benchfile"
settings=./setting_4ckpt

$gem5dir/build/RISCV/gem5.fast $gem5dir/configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB  --tracesetting=$settings  -c $readckpt "$option"
