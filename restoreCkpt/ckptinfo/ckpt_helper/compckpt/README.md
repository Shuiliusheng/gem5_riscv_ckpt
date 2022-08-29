### compress the first load information in ckpt files
- usage:
    ./readckpt.riscv compresslevel ckptinfo.log newckptinfo.log
    - ckptinfo.log: any compressed level ckpt files
    - compresslevel: default 2
        0: no compress
        1: only compress the first load with data map
        2: compress the data map with fastlz level 1
        3: compress the data map with fastlz level 2
 
