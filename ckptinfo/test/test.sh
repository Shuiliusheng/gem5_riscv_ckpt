#!/bin/bash
riscv64-unknown-linux-gnu-gcc big1.c -c -o big.o
riscv64-unknown-linux-gnu-gcc big.o -static -T link.lds -o big1.riscv
rm big.o

cp big1.riscv ~/experiment/gem5_dir/gem5_riscv_cpt_stack/ckptinfo/test/