#### create ckpt with gem5
#### 使用方法
1. 创建ckpt示例
    - 额外注意：编译程序时需要指定特定的link文件，具体示例在bench中存在(build.sh)
    - 该版本中不再使用gem5 debugflag, 因此可以执行使用gem5.fast来加速模拟过程
    - 该版本支持使用readckpt_new.riscv来执行ckpt文件，以此来重新创建新的ckpt文件
    ```shell
        ./build/RISCV/gem5.fast ./configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=settings -c bench/test.riscv --options=""
    ```
2. gem5之外的参数解释
    - (除此之外均为gem5原本的参数)
    - 参数：--ckptsettings=settingsfile 
        - 用于指定生成ckpt的一些参数信息，包括
            - mmapend/stacktop: 指定运行时的栈基址和map的最大位置
            - ckptprefix: 指定输出ckpt的目录和ckpt文件名前缀
            - ckptctrl: (startckpt interval warmupnum times), 文件中可以存在多个
                - startckpt: 从多少条指令开始创建ckpt
                - interval: 从startckpt开始间隔多少条指令创建一个ckpt
                - warmupnum: 每一个ckpt提前准备多少条指令用于恢复时的预热
                - times: 创建几个ckpt
            - readckpt: ./perlbench_ckpt_10000000.info 
              - readckpt: 用于指明当前是利用readckpt.riscv来读取一个已有的ckpt文件来重新创建不同的片段文件
              - ./perlbench_ckpt_10000000.info: 用于指明readckpt正在读取的ckpt文件。在执行过程中通过读取该文件中的系统调用信息来创建新的ckpt文件
            - strictLength: 指定该参数时，将不再寻找退出指令，而是选择直接在设定的指令长度停止信息收集，直接保存ckpt文件
        ```c
            //正常创建ckpt文件的settings文件样例
            //./build/RISCV/gem5.fast ./configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=settings -c bench/test.riscv
            mmapend: 261993005056
            stacktop: 270582939648
            ckptprefix: ./test
            ckptctrl: 10000 10000 1000 4
            ckptctrl: 60000 20000 1000 4    
    strictLength: 
        ```
        
        ```c
            //利用readckpt_new.riscv执行ckpt文件，并且重新创建新的ckpt文件的方法
            //.,... --ckptsetting=settings -c 利用readckpt_new.riscv --options="./perlbench_ckpt_10000000.info ./perlbench.riscv"
            mmapend: 7894835200
            stacktop: 8010178560
            ckptprefix: ./res/perlbench
            ckptctrl: 100 5000000 500000 4
            readckpt: ./perlbench_ckpt_10000000.info
        ```

#### 工作方式简介
1. 主要工作内容
    - 根据设置的ckpt起始指令和终止指令的信息，统计
        - 该间隔开始时寄存器的信息
        - 该间隔内的代码段访问信息
        - 该间隔内的内存区域访问信息
        - 该间隔内的第一次load的指令信息
    - 在执行过程中寻找能够结束该ckpt片段的特殊的退出指令
        - 该指令在ckpt片段内的最后一次系统调用后第一次出现
    
2. 主要步骤
    - 在执行到指定的指令数时，创建ckpt对象开始记录(class CkptInfo), 记录此时的寄存器状态
    - 记录到片段结束之前执行的指令地址，最后确定代码段区域 (set<uint64_t> textAccess)
    - 按顺序记录load/store/atomic三种存储操作的地址，最后用于确定访问的内存区域 (set<uint64_t> preAccess)
    - 遇到load指令时，判断该指令访问的地址是否在之前出现过(preAccess)，如果没有出现，则为first load
        - 需要注意：load指令有数据宽度，因此统计时将其拆分为单位为1byte的多条load进行判断
    - 记录到片段结束之前执行的系统调用的信息：系统调用指令地址，参数，系统调用号，修改的内存地址和数据，返回值
    - 寻找退出指令，结束ckpt的创建，根据记录的信息生成ckpt文件
    - 需要注意：
        - 由于退出指令的存在，使得记录的ckpt片段指令数会超过预定的指令数
        - 由于退出指令的存在，执行过程中同一个时间节点，可能会存在多个ckpt在创建


3. 创建ckpt文件 (saveCkptInfo)
    - 将textAccess和preAccess转换为区域信息，以4KB为基础单位，将其分别划分为多个区域(textrange, memrange)
    - 将记录的以1B为单位的first load进行合并，将其合并为以8B为单位的load指令（没有则为0）
    - 将ckpt基础信息和textrange, memrange, firstload(所有load的大小都为8B), syscallinfo写入文件中
    - 文件格式: prefix_ckpt_{startplace}.info
        - syscall information的格式介绍：
            - 在存储syscall信息之前会通过比较筛选出所有重复的系统调用信息，同时将每一次的系统调用通过索引(sys_indexes)将其指向所实际对应的系统调用信息位置(剔除重复信息后第几个系统调用)
            - 首先存储系统调用发生的次数
            - 接着存储每次系统调用对应的索引，以寻找实际系统调用的具体信息
            - 然后存储剔除重复信息后的系统调用信息，并且重用一些位域来压缩大小
            - 最后存储修改修改内存的所有系统调用所需要修改的地址(bufaddr)和数据
        | textinfo num              | 8      |
        |:-------------------------:|:------:|
        | textinfos(addr, size)     | num*16 |
        | sim start place           | 8      |
        | sim inst num & warmupnum  | 8      |
        | exit inst pc              | 8      |
        | exit cause & ckptlength   | 8      |
        | start npc                 | 8      |
        | start int regs            | 8*32   |
        | start ip regs             | 8*32   |
        | memrange num              | 8      |
        | memrange info(addr, size) | num*16 |
        | first load num            | 8      |
        | load (addr, data)         | num*16 |
        | **syscall info num**      |**8**   |
        | syscall_indexes           |sysnum*4|
        | pc                        | 8      |
        | num & data_size           | 8      |
        | p0                        | 8      |
        | ret                       | 8      |
        | hasret & data_offset      | 8      |
        |**bufaddr + syscall data** | 8+data_size|

4. 创建ckpt简介信息的文件 (saveDetailInfo)
    - 文件名: prefix_infor_{startplace}.txt
    - 内容：
        - ckptinfo: startnum, exitnum, length, warmup, pc, npc, exitpc
        - text range num, mem range num, first load num, syscallnum
        - integer register value
        - float register value
        - text range information
        - mem range information
        - running instruction information


5. 代码修改
    - 增加的文件: gem5_create/sim/ckpt_collect.cc  ckpt_collect.hh  ckptinfo.cc  ckptinfo.hh
    - 修改的文件:
        - ckpt配置: gem5_create/sim/process.cc SConscript arch/riscv/process.cc
        - 基本统计: gem5_create/cpu/simple/atomic.cc atomic.hh
        - 系统调用: gem5_create/sim/syscall_emul.hh sim/syscall_desc.cc arch/riscv/linux/se_workload.cc