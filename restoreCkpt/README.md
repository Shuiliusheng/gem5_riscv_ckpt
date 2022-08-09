### ckpt的读取-恢复-执行
1. 目录介绍
    - ckptinfo: ckpt文件处理和读取的主要目录
        - readckpt: 用于读取ckpt和恢复ckpt片段的执行环境，支持设置最大运行指令数和warmup指令数
        - newreadckpt: 用于读取ckpt和恢复ckpt片段的执行环境，但是不需要任何硬件支持，也不具备设置最大运行指令数和warmup的功能
        - showckpt: 用于读取ckpt文件内的信息，不直接恢复ckpt的执行环境，可以本地编译执行
    - riscv-pk: 用于verilator和spike执行时使用，主要是在原本pk的基础上修改了mmap和brk设置相关的内容
    - fpga_support: 用于支持一些特殊功能时的硬件支持，例如设置warmup指令和最大运行指令和增加四个临时寄存器的读写功能
        - boom_scala: 修改之后的boom代码
        - rocket-chip: 修改的rocket代码，主要改动了dts配置内存大小的地方
    - gem5_support: (暂时没有什么用处)
       - 在gem5的riscv译码逻辑中，增加了对临时寄存器读写指令的支持，同时支持了四个临时寄存器的设计，能够恢复执行readckpt文件

2. 使用方法
    - ckptinfo/readckpt: 
        - ./readckpt.riscv bencn_ckpt_{start_place}.info bench.riscv
        - 已经提供了统计恢复执行后，bench片段执行的周期和指令数的统计
            - 为了避免统计takeOverFunc函数内程序执行对性能事件影响，因此目前是定义了一些全局cycles和insts计数器，并且在进入和离开takeOverFunc函数时分别统计一次，以此做差来剔除掉该函数内部的事件
            - 若需要增加新的事件统计，可参考runningInfo->cycles等参数的设置方式
        - read_ckptinfo函数为恢复切片的函数，takeoverSyscall则为接管系统调用和退出指令的函数
        - 执行结束时机：由ckpt.info文件中指定的运行指令数来决定合适退出执行
        - 通过修改硬件(boom_stop的设计)提供了设置warmup和最大运行指令数的设置，设置接口为init_start(maxinst, warmup)，在read_ckptinfo加载完所有需要使用的切片环境后被调用
            - 当程序运行到warmup指令数后，清空硬件所有计数器的值；继续运行maxinst指令数时，跳转到指定的退出函数入口，结束执行
            - 执行到设定的指令数后跳转到指定的地址
              - 地址由__sampleFucAddr，即sample_func函数的入口地址
              - 具体地址在硬件上的设置由：ctrl.h中的init_start完成
        - bencn_ckpt_{start_place}.info生成时已经提供了warmup指令数和切片原本需要的指令数，如果没有设置，则采用simNum
        - 需要硬件支持的部分：
          - 需要硬件支持warmup和运行指令数设置
          - 使用了boom_stop中设置临时寄存器并根据临时寄存器跳转的功能，来实现readckpt和ckpt中指令之间的来回跳转
  
    - ckptinfo/newreadckpt:
        - 完成的工作和readckpt一致，均是根据ckpt.info文件恢复记录的程序片段
        - 差别：
          - 不支持设置warmup和maxinst，同时没有使用硬件上的临时寄存器完成跳转，因此能够完全不需要硬件支持
          - newreadckpt可以直接在任何没有修改的riscv环境中支持，例如fpga+linux，gem5，verilator+pk(pk需要修改一部分代码)
          - newreadckpt中通过jal+jalr+ld的组合来实现readckpt和ckpt中指令之间的来回跳转
  
    - ckptinfo/showckpt: 仅读取和显示ckpt内的信息，不恢复执行
        - ./showckpt bencn_ckpt_{start_place}.info bench.riscv
        - 基本文件内容和readckpt基本一致
        - 在本地gcc编译和运行，而不是使用riscv的工具链


3. 工作原理
    - 基本思想：在不影响readckpt本身的代码段和栈空间的前提下，将ckpt的信息恢复到加载器的进程空间中
        - 避免代码段影响：将readckpt的代码段放置在和ckpt中program代码段不一样的位置
          - ckpt中的program正常编译时，代码段从0x1000开始；brk则是从数据段结束位置开始；（一般情况数据段不会到0x1000000）
          - readckpt的代码段通过链接脚本将其指定到0x1000000的位置，brk也是从数据段结束位置开始，但是数据段会避免超过0x1400000
          - gem5在执行program生成ckpt时，将起始的brk位置设定在0x1400000位置，因此program在运行时不会使用0x1000000-0x1400000的空间，所以两者的代码段将不会发生冲突
          - 如果program的数据段超过了0x1000000或者在执行过程中使用了0x1000000-0x1400000的空间，则可以重新挑选一段空间给readckpt使用即可
        - 避免影响栈空间：gem5在执行bench生成ckpt时，指定stack_top和mapend来将栈空间放在其他地方，从而避免和加载器的栈相互冲突
          - （需要注意riscv-pk的初始栈顶和fpga的栈顶设置不一样，会更小，因此生成ckpt时需要单独设置）
        - 恢复代码段：读取ckpt中代码区域的信息，将执行需要的代码区域加载到对应地址
        - 恢复访存空间：读取ckpt中mem区域的信息，使用map对这些区域的进行分配
        - 恢复firstload：映射内存空间后，将first load的数据写入对应的地址
        - 加载系统调用信息：读取ckpt中的系统调用信息，将其放在内存某块区域，用于处理系统调用使用
        - 利用临时寄存器完成所有跳转功能的需求：
          - 跳转到起始地址：ckpt记录的第一条指令的地址
          - 系统调用到接管函数的跳转：用于接管和重现系统调用
          - 接管函数跳转到系统调用返回地址（系统调用的下一条指令）
        - 恢复整数和浮点寄存器的状态
        - 跳转到ckpt记录的第一条指令的地址开始执行


#### 具体设计和实现（以readckpt的实现为例）
根据ckpt_{startplace}.info初始化执行环境，设置系统调用的接管信息，启动测试程序

1. 定义一些全局变量，记录一些信息
    - 声明RunningInfo结构体，并定义该结构体的全局变量，用于在takeOverFunc函数中使用。目前存放的信息主要
        - 包括系统调用接管时使用的信息
        - 为了统计执行信息的计数器
        ```c
            typedef struct{
                uint64_t exitpc;        //退出指令的PC
                uint64_t exit_cause;    //退出的原因：系统调用/特定退出指令
                uint64_t syscall_info_addr; //所有系统调用在内存中的地址信息
                uint64_t nowcallnum;    //当前将被指令的系统调用是第几个
                uint64_t totalcallnum;  //一共将执行多少个系统调用
                //perf counter information
                uint64_t cycles;        
                uint64_t insts;         
                uint64_t lastcycles;
                uint64_t lastinsts;
                uint64_t startcycles;   //benchmark的执行周期
                uint64_t startinsts;    //benchmark的执行指令
            }RunningInfo;
        ```
    - 定义一段内存，用于存放ckpt记录的程序执行过程中的浮点和整数寄存器内容，以及readckpt原本寄存器的内容
        ```c
            extern uint64_t readckpt_regs[32];
            extern uint64_t program_intregs[32];
            extern uint64_t program_fpregs[32];
            // 通过这些数组的名字，将能够直接获取对应的地址
            // 为了避免la转换为gp+imm数的形式，需要额外在此之前定义一段空间
            // la a0, program_intregs
        ```
    - 定义takeoverSyscall函数的入口地址：takeOverAddr
        ```c
            //ckptinfo.h
            extern uint64_t takeOverAddr;
            //takeover_syscall.cpp
            extern unsigned long long __takeOverSys_Addr;
            uint64_t takeOverAddr = (uint64_t)&__takeOverSys_Addr;
            void takeoverSyscall()
            {
                asm volatile("__takeOverSys_Addr: ");
                Save_ProgramIntRegs();
                Load_ReadCkptIntRegs();
            }
        ```
    - 声明一些汇编指令序列，用于保存和恢复寄存器
        ```c
            //ckptinfo.h
            #define Save_ProgramIntRegs()
            #define Load_ProgramIntRegs()
            #define Save_ProgramFpRegs()
            #define Load_ProgramFpRegs()

            #define Save_ReadCkptIntRegs()
            #define Load_ReadCkptIntRegs() //仅恢复部分寄存器即可, ra, sp, gp, tp, fp
        ```

2. 加载测试程序的代码段
    - 读取ckpt_{startplace}.info，获取程序执行时所需要的代码区域信息textinfo
        - 代码段的所有区域都会使用mmap进行分配
        - 仅加载textinfo记录的使用到的代码区域，以4KB为单位划分
        - 也可以加载program的所有代码段
    - 读取测试程序的elf文件，获取需要代码段的段地址和段大小
        - 对段地址进行对齐，使其以4KB对齐
        - 对段大小进行扩充，使其以4KB为单位
        - 将扩充之后的段信息记录在 (text_seg.addr, text_seg.size) 中
    - 使用mmap对区域(text_seg.addr, text_seg.size)进行映射，使其可读可写可执行
    - 根据textinfo中的信息，计算文件偏移，将代码区域的数据从文件加载到对应的内存中

3. 加载ckpt文件，启动测试程序
    - 打开ckpt_{startplace}.info文件，首先跳过关于textRange的开头信息
    - 读取siminfo, 将exitpc和exit_cause放入runningInfo中，同时打印ckpt的信息
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
    - 读取npc, 读取int_regs, fp_regs到program_intregs和program_fpregs
    - 读取memrange信息，对所有需要的内存区域使用mmap进行映射，权限为可读可写
        - 需要注意: 为了避免需要映射stack所处的区域，而导致启动程序的栈区出现问题，需要在gem5创建ckpt时，就要将gem5的测试环境中的stack_top设置为恢复ckpt环境中可以直接使用mmap进行映射的空间。（简单理解就是create时让stack和mmap的区域更小，restore的时候才能够恢复这些区域）
        ```c
            typedef struct{
                uint64_t addr;
                uint64_t size;
            }MemRangeInfo;
        ```
    - 读取firstload的信息，并且根据地址和访问数据的大小，初始化对应地址的数据
        - ckpt.info文件中记录的load信息中数据的宽度均为8B
        ```c
            typedef struct{
                uint64_t addr;
                uint64_t data;
            }LoadInfo;
        ```
    - ### 至此执行环境基本初始化完成
    - 继续读取ckpt_{startplace}.info剩余内容， 加载系统调用信息
        - 确定该系统调用信息的全部大小（此处到文件结尾的大小）
        - 使用mmap映射相同大小的一段内存区域，将文件中所有内容加载到该区域
        - 将该内存区域的地址记录在runningInfo.syscall_info_addr中
        - 根据文件的前8个字节，初始化runningInfo.totalcallnum

    - 处理执行过程中会遇到的一些跳转：跳转到benchmark的第一条指令执行；系统调用的跳转和返回
        - 为了修改寄存器的内容，readckpt的设计中选择使用boom_stop中提供的临时寄存器功能
          ```c
            //define.h
            #define Set_TempReg(srcreg, rtemp)   "addi x0, " srcreg ", 9+" #rtemp " \n\t"  
            #define SpecialInst_URet(rtemp)     "addi x0, x" rtemp ", 64 \n\t" 
            //设置第n个临时寄存器的内容为value
            #define SetTempReg(value, n) asm volatile( \
                "mv t0, %[rtemp1]  \n\t"    \
                Set_TempReg("t0", n)         \
                : :[rtemp1]"r"(value)         \
            );
            //根据第n个临时寄存器的内容进行跳转
            #define JmpTempReg(n) asm volatile( SpecialInst_URet(#n) );
          ```
        - 系统调用的跳转的处理：从program跳转到readckpt的代码段
          - 提前将takeOverAddr放入临时寄存器3中
          - 将ckpt中遇到的系统调用指令的ecall指令替换为JmpTempReg(3)的二进制即可
          - 退出指令也采用类似的方案，交由takeOverFunc完成对退出的判断
        - 跳转到ckpt第一条指令和ecall的下一条指令：从readckpt跳转到program的代码段
          - 将目标地址写入临时寄存器
          - 利用JmpTempReg完成跳转
          - 为了避免和sample_func使用的0号临时寄存器冲突，因此选择使用其它的临时寄存器
    
    - 根据ckpt文件中包含的系统调用数据，第一次决定是否更改exitInst为JmpTempReg(3)
        - 如果runninginfo.totalcallnum == 0，即没有出现系统调用，则此时就将exitpc对应的指令替换为JmpTempReg(3)
        - 之后执行到该指令时，由syscalltakeover程序结束执行
    - 将NPC放入临时寄存器(rtemp0): SetTempReg(npc, 2);
    - 将takeoverSyscall函数中的入口地址放入临时寄存器(rtemp3) （不是函数的第一条指令，而是保存上下文的第一条指令）: SetTempReg(takeOverAddr, 3);
    - 保存支持readckpt中takeOverFunc函数运行的一些必要寄存器到readckpt_regs中，包括ra, sp, fp, tp, fp
    - 恢复测试程序入口的寄存器状态
    - 使用JmpTempReg(2)指令，跳转到入口地址，开始执行


4. 接管和复现系统调用的影响
    - 介绍：在加载完代码段到内存之后，已经将所有ecall替换为JmpTempReg(3)，因此在代码执行过程中，原本的系统调用会直接跳转到该函数执行
    - 函数内的工作：
        - step1: 保存当前寄存器的状态（目前只保存了整数寄存器，浮点还没被用到）
        - step2: 恢复启动程序的部分寄存器的状态。如果不恢复，无法使用printf等函数，同时也无法获取全局变量
        - step4: 判断当前系统调用的次数是否已经超过了记录的runninginfo.totalcallnum次数。如果超出，则结束执行，即ckpt执行错误或者执行结束
        - step4: 读取runninginfo结构体，获取当前系统调用的次数，以此来找到当前系统调用对应的信息
            ```c
                //ckpt中syscall信息的存储格式决定了当前的读取方法
                //获取syscall信息的基址
                uint64_t infoaddr = runninginfo.syscall_info_addr + 8 + runninginfo.totalcallnum*4;
                //获取当前系统调用对应于哪一个系统调用信息，sysidxs决定了此信息
                uint32_t *sysidxs = (uint32_t *)(runninginfo.syscall_info_addr + 8);
                //获取当前系统调用实际信息的存储地址
                SyscallInfo *infos = (SyscallInfo *)(infoaddr + sysidxs[runninginfo.nowcallnum]*sizeof(SyscallInfo));

                //需要单独处理的系统调用信息：
                //系统调用号
                uint64_t sysnum = infos->num >> 32;
                //系统调用数据偏移
                uint64_t data_offset = infos->data_offset >> 8;
                //数据大小
                uint64_t data_size = (infos->num<<32) >> 32;
                //是否存在返回值
                uint64_t hasret = infos->data_offset % 256;
            ```
        - step5: 判断当前系统调用号是否和infos中记录的系统调用号一样。如果不一样，则执行错误，退出
        - step6: 判断当前系统调用是否需要修改内存数据（infos->data_offset != 0xffffffff）
            - 根据bufaddr和记录的数据大小，直接向内存写数据
        - step7: 如果该系统调用有返回值，则将其写回（program_intregs[10] = infos->ret）
          - 如果没有返回值，则将info中继续的a0值写回到program_intregs[10]中（program_intregs[10] = infos->p0）
        - step8: 更新runninginfo.nowcallnum（runninginfo.nowcallnum++）
        - step9: 判断是否已经是最后一个系统调用。如果是，则将代码段中的exitpc对应的指令替换为jmp rtemp3指令
        - step10: 将系统调用的返回地址写入rtemp2 (infos->pc + 4)
        - step11: 恢复程序的寄存器状态
        - step12: jmp rtemp2，跳回程序继续执行


#### newreadckpt与readckpt的区别：不需要硬件任何支持
1. 没有使用boom_stop提供的设置warmup和执行指令数的支持
2. 没有使用boom_stop提供的设置临时寄存器来完成各种跳转需求
3. 跳转的实现
    - 基本思路：利用ckpt中记录的program使用的代码区域信息，寻找一些空闲内存作为中间踏板，完成readckpt和program之间的跳转
    - 需要跳转的地址：start_pc, ecall_pc(ecall_pc+4), exit_pc
    - 具体设计：
      - 为所有需要跳转的地址寻找就近的空闲区域，使其能够利用jal完成在目标地址和空闲区域之间相互跳转
      - 对于ecall指令：
        - 将其替换为jal指令使其跳转到寻找到的空闲区域
        - 使用lui+addi将a0寄存器设置为指定的内存地址（uint64_t takeOverAddr的地址）
        - 使用ld a0, 0(a0)指令读取该地址，获取takeOverFunc的入口地址
        - 使用jalr x0, 0(a0)完成跳转
        - a0寄存器的内容由takeOverFunc完成恢复
        - （lui+addi+ld+jalr已经提前设置在对应的内存）
      - 对于跳转到ecall返回地址和ckpt第一条指令的情况
        - 在readckpt的代码中，首先利用lui+addi将a0设置为指定的内存地址tempMemory，
        - 在该内存地址出放置需要跳转的目标地址，具体的目标地址为事先确定的空闲区域的一块地址
        - 利用ld a0, 0(a0)指令读取该地址，使用jalr x0, 0(a0)跳转到该地址
        - 该地址存储的指令用于根据program_intregs恢复a0寄存器的值，并且利用jal跳转到最后实际的目标地址。（这些指令也是在恢复ckpt执行之前完成设置的）