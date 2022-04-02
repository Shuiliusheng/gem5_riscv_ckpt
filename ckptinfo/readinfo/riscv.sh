#!/bin/bash

riscv64-unknown-linux-gnu-gcc readinfo.cpp -static -O2 -o read.riscv
#riscv64-unknown-linux-gnu-objdump -D read.riscv >read.s
