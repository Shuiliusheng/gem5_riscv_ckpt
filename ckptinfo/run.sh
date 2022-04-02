#!/bin/bash

build/RISCV/gem5.opt ./configs/example/se.py -c ckptinfo/readinfo/read.riscv --options="ckptinfo/info/ckpt_3000_6000.info ckptinfo/test/main"
build/RISCV/gem5.opt --debug-flag=ShowMemInfo,ShowRegInfo,ShowSyscall  ./configs/example/se.py --ckptinsts=3000 -c ckptinfo/test/main
build/RISCV/gem5.opt --debug-flag=ShowDetail ./configs/example/se.py -c ckptinfo/readinfo/read.riscv --options="ckptinfo/info/ckpt_3000_6000.info ckptinfo/info/ckpt_syscall_3000_6000.info ckptinfo/test/main"


build/RISCV/gem5.opt --debug-flag=ShowDetail  ./configs/example/se.py --ckptinsts=6000 -c ckptinfo/test/main

build/RISCV/gem5.opt --debug-flag=ShowDetail ./configs/example/se.py -c ckptinfo/readinfo/read.riscv --options="ckptinfo/info/ckpt_6000_9000.info ckptinfo/info/ckpt_syscall_6000_9000.info ckptinfo/test/main"

build/RISCV/gem5.opt --debug-flag=ShowDetail ./configs/example/se.py -c ckptinfo/readinfo/read.riscv --options="ckptinfo/info/ckpt_3000_6000.info ckptinfo/info/ckpt_syscall_3000_6000.info ckptinfo/test/main"

build/RISCV/gem5.opt ./configs/example/se.py -c ckptinfo/readinfo/read.riscv --options="ckptinfo/info/ckpt_6000_9000.info ckptinfo/info/ckpt_syscall_6000_9000.info ckptinfo/test/main"


build/RISCV/gem5.opt --debug-flag=ShowDetail  ./configs/example/se.py --ckptinsts=12000 -c ckptinfo/test/main

build/RISCV/gem5.opt --debug-flag=ShowDetail ./configs/example/se.py --ckptinsts=3000 -c ckptinfo/readinfo/read.riscv --options="ckptinfo/info/ckpt_21000_24000.info ckptinfo/info/ckpt_syscall_21000_24000.info ckptinfo/test/main"

build/RISCV/gem5.opt --debug-flag=ShowMemInfo,ShowRegInfo,ShowSyscall --debug-file=sysinfo.log ./configs/example/se.py --ckptinsts=5000 -c ckptinfo/test/sys.riscv


build/RISCV/gem5.opt ./configs/example/se.py --ckptinsts=5000 -c ckptinfo/readinfo/read.riscv --options="ckptinfo/info/ckpt_20000_25000.info ckptinfo/info/ckpt_syscall_20000_25000.info ckptinfo/test/sys.riscv"


