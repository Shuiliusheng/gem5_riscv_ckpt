#!/bin/bash
riscv64-unknown-linux-gnu-gcc main.c -c -o main.o
riscv64-unknown-linux-gnu-gcc main.o -static -T link.lds -o main
