### ckpt的读取-恢复-执行
1. 目录介绍
    - ckptinfo: ckpt文件处理和读取的主要目录
        - readckpt: 不需要硬件任何修改的ckpt加载器，遇到记录的退出指令时退出执行
        - maxinst_readckpt: 在readckpt的基础上，增加了支持设置warmup指令和最大运行指令的支持，需要硬件支持（fpga_support）
        - showckpt: 用于读取ckpt文件内的信息，不直接恢复ckpt的执行环境，可以本地编译执行
        - bench: 一些编译好的可执行程序
    - fpga_support: 用于支持一些特殊功能时的硬件支持，例如设置warmup指令和最大运行指令，其他情况不需要
        - riscv-pk: 用于verilator和spike执行时使用
        - boom_scala: 修改之后的boom代码
        - rocket-chip: 修改的rocket代码，主要改动了dts配置内存大小的地方

2. 使用方法
    - ckptinfo/readckpt: 不需要硬件支持的加载器
        - ./readckpt.riscv bencn_ckpt_{start_place}.info bench.riscv
        - 已经提供了统计恢复执行后，bench片段执行的周期和指令数的统计
            - 若需要增加新的事件统计，需要在RunningInfo结构体中增加新的计数器，具体实现可参考runningInfo->cycles等参数的设置方式
        - read_ckptinfo函数为恢复切片的函数，takeoverSyscall则为接管系统调用和退出指令的函数
        - 执行结束时机：遇到退出指令时结束执行
        - 时机执行指令数：simNum
    - ckptinfo/maxinst_readckpt: 不需要硬件支持的加载器, 支持warmup和特定指令数退出
        - ./readckpt_max.riscv bencn_ckpt_{start_place}.info bench.riscv
        - 通过修改硬件提供了设置warmup和最大运行指令数的设置，设置接口为init_start(maxinst, warmup)，在read_ckptinfo加载完所有需要使用的切片环境后被调用
            - 当程序运行到warmup指令数后，清空硬件所有计数器的值；继续运行maxinst指令数时，跳转到指定的退出函数入口，结束执行
            - 执行到maxinst时跳转的入口地址：ctrl.h/exit_fuc()/__exitFucAddr ; 入口地址的设置在init_start中完成
        - bencn_ckpt_{start_place}.info生成时已经提供了warmup指令数和切片原本需要的指令数，如果没有设置，则采用simNum
    - ckptinfo/showckpt: 仅读取ckpt内的信息，不恢复执行
        - ./showckpt bencn_ckpt_{start_place}.info bench.riscv
        - 基本文件内容和readckpt基本一致


3. 工作原理
    - 基本思想：在不影响加载器代码段和栈空间的前提下，将ckpt的信息恢复到加载器的进程空间中
        - 避免代码段影响：bench通过链接脚本，将代码段的起始地址设置为0x200000，从而能够避免和加载器代码段冲突
        - 避免影响栈空间：gem5在执行bench生成ckpt时，指定stack_top和mapend来将栈空间放在其他地方，从而避免和加载器的栈相互冲突
        - （需要注意riscv-pk的初始栈顶和fpga的栈顶设置不一样，会更小，因此生成ckpt时需要单独设置）
        - 恢复代码段：读取ckpt中代码区域的信息，将执行需要的代码区域加载到对应地址
        - 恢复访存空间：读取ckpt中mem区域的信息，使用map对这些区域的进行分配
        - 恢复firstload：映射内存空间后，将first load的数据写入对应的地址
        - 加载系统调用信息：读取ckpt中的系统调用信息，将其放在内存某块区域，用于处理系统调用使用
        - 确定跳转信息：
            - 如何跳转到ckpt的入口地址
            - ecall如何跳转到takeoverSyscall函数入口地址
            - 如何跳转到ecall的下一条指令
        - 恢复整数和浮点寄存器的状态
        - 跳转到ckpt的入口地址开始执行


#### 具体设计和实现
根据ckpt_{startplace}.info初始化执行环境，设置系统调用的接管信息，启动测试程序

1. 映射一段固定地址的区域，用于存放执行过程中需要用到的信息和作为临时区域使用
    - 固定地址的目的在于可以直接使用固定地址访问到，不会受限于当前处于启动程序还是测试程序中
    - 目前存放的信息主要
        - 包括系统调用接管时使用的信息
        - 用于保存上下文的寄存器信息
        - 为了统计执行信息的计数器
        - 为了完成对系统调用跳转支持的信息
    ```c
        typedef struct{
            uint64_t exitpc;        //退出指令的PC
            uint64_t exit_cause;    //退出的原因：系统调用/特定退出指令
            uint64_t syscall_info_addr; //所有系统调用在内存中的地址信息
            uint64_t nowcallnum;    //当前将被指令的系统调用是第几个
            uint64_t totalcallnum;  //一共将执行多少个系统调用
            uint64_t intregs[32];
            uint64_t fpregs[32];    //benchmark的寄存器信息
            uint64_t oldregs[32];   //加载程序的寄存器信息
            uint64_t cycles;        
            uint64_t insts;         
            uint64_t lastcycles;
            uint64_t lastinsts;
            uint64_t startcycles;   //benchmark的执行周期
            uint64_t startinsts;    //benchmark的执行指令
            uint32_t exitJmpInst;   //退出指令要被替换为的跳转指令
            JmpRepInfo *sysJmpinfos;   //为了跳转到系统调用返回地址所需要使用的跳转指令
        }RunningInfo;
        uint64_t alloc_vaddr = (uint64_t)mmap((void*)RunningInfoAddr, 4096*2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
    ```

2. 加载测试程序的代码段
    - 读取ckpt_{startplace}.info，获取程序执行时所需要的代码区域信息textinfo
        - 代码段的所有区域都会使用mmap进行分配
        - 仅加载textinfo记录的使用到的代码区域，以4KB为单位划分
        
    - 读取测试程序的elf文件，获取需要代码段的段地址和段大小
        - 对段地址进行对齐，使其以4KB对齐
        - 对段大小进行扩充，使其以4KB为单位
        - 将扩充之后的段信息记录在 (text_seg.addr, text_seg.size) 中
    - 使用mmap对区域(text_seg.addr, text_seg.size)进行映射，使其可读可写可执行
    - 根据textinfo中的信息，计算文件偏移，将代码区域的数据从文件加载到对应的内存中

3. 加载ckpt文件，启动测试程序
    - 打开ckpt_{startplace}.info文件，首先跳过关于textRange的开头信息
    - 初始化runinfo结构体
        - RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
    - 读取siminfo, 将exitpc和exit_cause放入runinfo中，同时打印ckpt的信息
        - exit_cause共64位，其中低两位用于指明实际的cause，其余位用于作为片段原本要求的指令数量
        - simNum共64位，低32位用于表示片段执行到退出指令的指令数，高32位用于表示warmup的指令数量
            - 如果没有实现warmup功能，可以忽略
        ```c
            typedef struct{
                uint64_t start;
                uint64_t simNum;
                uint64_t exitpc;
                uint64_t exit_cause;
            }SimInfo;
        ```
    - 读取npc, 读取int_regs, fp_regs到runinfo中
    - 读取memrange信息，对所有需要的内存区域使用mmap进行映射，权限为可读可写
        - 需要注意: 代码段已经进行了映射，因此需要对memrange中的区域进行检查和拆分，避免重复映射
        - 需要注意1: 为了避免需要映射stack所处的区域，而导致启动程序的栈区出现问题，需要在gem5创建ckpt时，就要将gem5的测试环境中的stack_top和mmap_end设置为恢复ckpt环境中可以直接使用mmap进行映射的空间。（简单理解就是create时让stack和mmap的区域更小，restore的时候才能够恢复这些区域）
        ```c
            typedef struct{
                uint64_t addr;
                uint64_t size;
            }MemRangeInfo;
        ```
    - 读取firstload的信息，并且根据地址和访问数据的大小，初始化对应地址的数据
        - 需要注意：和memrange中的注意事项一样，也需要避免恢复这些数据时直接覆盖了当前的栈区，导致程序直接崩溃
        ```c
            typedef struct{
                uint64_t addr;
                uint64_t size;
                uint64_t data;
            }LoadInfo;
        ```
    - 至此执行环境基本初始化完成
    - 继续读取ckpt_{startplace}.info剩余内容， 加载系统调用信息
        - 确定该系统调用信息的全部大小（此处到文件结尾的大小）
        - 使用mmap映射相同大小的一段内存区域，将文件中所有内容加载到该区域
        - 将该内存区域的地址记录在runinfo.syscall_info_addr中
        - 根据文件的前8个字节，初始化runinfo->totalcallnum

    - 处理执行过程中会遇到的一些跳转：跳转到benchmark的第一条指令执行；系统调用的跳转和返回
        - 基础信息：
            - 为了避免benchmark和loader程序的代码段重合，因此benchmark的代码段放在0x200000开始的地方
            - jal x0, imm21 最大能够跳转+/-1MB的空间
        - 方法：寻找代码段内部空余的位置进行多次跳转
        - 首先：根据ckpt_{startplace}.info中记录的代码区域的信息，找到所有代码段中剩余的内存信息freeRanges
        - 其次；统计所有会使用跳转功能的地址：npc, exitpc, syscall
        - 接着：根据freeRanges，寻找能够最终覆盖所有跳转地址的跳转路径（多个中间节点midpoints）
        - 然后：由系统调用跳转到takeoverSyscall函数中的入口地址
            - 将所有系统调用指令根据midpoints的信息替换为跳转到对应中间点的跳转指令（在执行之前就可以完成替换）
            - 记录退出指令exitpc所需要的跳转指令信息，用于实际需要替换时再替换
        - 最后：takeoverSyscall函数跳转到系统调用返回地址（在执行过程中才可以完成替换）
            - 根据midpoints的信息，寻找跳转到系统调用返回指令所需要的跳转指令序列和指令的地址信息JmpRepInfo。在实际处理完系统调用时，在根据这些信息完成跳转指令的写入
            - 对于npc也是同样的处理方法updateJmpInst
    
    - 根据ckpt文件中包含的系统调用数据，第一次决定是否更改exitInst
        - 如果runinfo->totalcallnum == 0，即没有出现系统调用，则此时就将exitpc对应的指令替换为jmp指令
        - 之后执行到该指令时，由syscalltakeover程序结束执行
    - 保存启动程序运行的一些必要寄存器到runinfo->oldregs中，包括sp, fp, tp, fp
    - 更新为了跳转到npc所需要的跳转指令， 恢复测试程序入口的寄存器状态， 利用jmp指令跳转到npc开始执行

4. 接管系统调用
    - 介绍：在加载完代码段到内存之后，已经将所有ecall替换为jmp takeoverSyscall, 因此在代码执行过程中，原本的系统调用会直接跳转到该函数执行
    - 函数内的工作：
        - step1: 保存当前寄存器的状态（目前只保存了整数寄存器，浮点还没被用到）
        - step2: 恢复启动程序的部分寄存器的状态 (如果不恢复，无法使用printf等函数)
        - step3: 初始化runinfo结构体，获取当前系统调用的次数，以此来找到当前系统调用对应的信息
            ```c
                RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
                uint64_t saddr = runinfo->syscall_info_addr;
                SyscallInfo *infos = (SyscallInfo *)(saddr+8+runinfo->nowcallnum * sizeof(SyscallInfo));
            ```
        - step4: 判断当前系统调用的次数是否已经超过了记录的runinfo->totalcallnum次数。如果超出，则exit，即ckpt执行错误或者执行结束
        - step5: 判断当前系统调用号是否和infos中记录的系统调用号一样。如果不一样，则执行错误，退出
        - step6: 判断当前系统调用是否需要修改内存数据（infos->data_offset != 0xffffffff）
            - read和write单独处理。当write的fd==1时，则直接输出到屏幕
            - 其他情况，根据bufaddr和记录的数据大小，直接向内存写数据
        - step7: 如果该系统调用有返回值，则将其写回（runinfo->intregs[10] = infos->ret）
        - step8: 更新runinfo->nowcallnum（runinfo->nowcallnum++）
        - step9: 判断是否已经是最后一个系统调用。如果是，则将代码段中的exitpc对应的指令替换为runinfo->exitJmpInst
        - step10: 根据系统调用当前的次数和JmpRepInfo，更新跳转指令序列，使其跳转回系统调用的后一条地址
        - step11: 恢复测试程序的寄存器状态
        - step12: 利用跳转指令，完成实际的跳转

