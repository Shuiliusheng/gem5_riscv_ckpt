#!/bin/bash

gem5_dir=/home/gem5_src/gem5
settingfile=./setting_4test
program=./test.riscv

$gem5_dir/build/RISCV/gem5.fast $gem5_dir/configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB --tracesetting=$settingfile -c $program
