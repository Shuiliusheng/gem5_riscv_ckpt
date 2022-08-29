### 读取生成的ckpt文件，加载测试程序运行，需要硬件提供四个额外的逻辑寄存器，用于避免修改原本的寄存器
1. ckptinfo.h: ckpt加载器的头文件，用于定义一些struct, 汇编代码, 函数, extern变量
     - 常量定义
    ```c
        #define ECall_Replace 0x04018013
        #define Cause_ExitSysCall 1 
        #define Cause_ExitInst 2
    ```
    - 结构体定义
    ```c
        typedef struct{
            uint64_t addr; 
            uint64_t size;
        }MemRangeInfo; //存储区域信息，用于textrange和memrange

        typedef struct{
            uint64_t pc;
            uint64_t num; //高32位为系统调用号，低32位为系统调用修改的内存大小
            uint64_t p0;  //a0寄存器
            uint64_t ret; 
            uint64_t data_offset;   //高56位为实际数据在文件中的偏移;最低位用于表示该系统调用是否有返回值
        }SyscallInfo; //每一个系统调用包含的信息

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
    - 内联汇编定义
    ```c
        //设置和根据临时寄存器跳转
        #define SetTempReg(value, n)
        #define JmpTempReg(n)

        //声明一些汇编指令序列，用于保存和恢复寄存器
        #define Save_ProgramIntRegs()
        #define Load_ProgramIntRegs()
        #define Save_ProgramFpRegs()
        #define Load_ProgramFpRegs()
        #define Save_ReadCkptIntRegs()
        #define Load_ReadCkptIntRegs() //仅恢复部分寄存器即可, ra, sp, gp, tp, fp
    ```
    - 全局函数和变量的定义
    ```c
        //takeoverSyscall函数的入口地址（并不是函数的第一条指令）
        extern uint64_t takeOverAddr;
        //代码段的信息，代码段的所有区域都会被映射到内存中
        extern MemRangeInfo text_seg;   
        //存储一些takeOverFunc函数需要使用的信息
        extern RunningInfo runningInfo;
        //用于存储寄存器
        extern uint64_t readckpt_regs[32];
        extern uint64_t program_intregs[32];
        extern uint64_t program_fpregs[32];
        //接管系统调用并处理的函数
        void takeoverSyscall(); 
        //加载elf文件的代码段
        uint64_t loadelf(char * progname, char *ckptinfo);
        //读取ckpt文件中的信息，并恢复benchmark的执行环境
        void read_ckptinfo(char ckptinfo[]);
    ```

2. readckpt.cpp: main函数入口，主要负责初始分配一些固定的内存区域
    - main(int argc, char **argv)
      - argv[1]: ckpt文件名; argv[2]: benchmark的elf文件
      - 调用loadelf(argv[2], argv[1])
      - 调用read_ckptinfo(argv[1])

3. elf_load.cpp: 分配benchmark的代码段，并且加载代码到内存中
    - uint64_t loadelf(char * progname, char *ckptinfo)
      - 解析ELF文件，获取代码段的信息
      - 读取ckpt文件，获取使用的代码区域信息，并从elf文件中将这些信息加载到内存中
  
4. ckpt_load.cpp: 完成对ckpt的读取，对benchmark的环境恢复
    - void read_ckptsyscall(FILE *fp)
      - 从ckpt文件中读取所有系统调用的信息放到内存中
      - 将所在的内存地址记录在runningInfo.syscall_info_addr中，用于接管系统调用时处理
      - 设置runningInfo.nowcallnum和runningInfo.totalcallnum
      - 遍历所有的系统调用信息，根据系统调用ecall指令的pc，将ecall指令替换为JmpTempReg(3)指令
    - void alloc_memrange(FILE *p)
      - 从ckpt文件中读取所有的内存区域信息MemRangeInfo
      - 将所有的内存区域使用map函数进行映射
      - 由于之前代码段已经完成了映射，因此判断当前内存区域是否和代码段有部分重叠，如果有则需要单独处理和映射
      - （访问执行过程中访问了代码段的部分区域，如果不映射代码段所有区域，那就需要单独比较所有记录的代码区域）
    - void setFistLoad(FILE *p)
      - 从ckpt文件中读取所有的first load信息，并且将其恢复到内存中
      - 利用fastlz中的解压缩函数，将原始数据解压
      - 解压之后的数据格式为：addr, size, datamap, data
    - void read_ckptinfo(char ckptinfo[])
      - 主要任务：读取ckpt文件，恢复所有执行环境，完成跳转转换，启动benchmark执行
      - 读取ckpt文件的头部，获取程序片段的起始指令数，片段指令数（到退出指令为止），设置的片段长度，warmup的指令数，退出指令的pc，退出的原因，片段的第一条指令pc地址
      - 从ckpt文件中获取整数和浮点寄存器的数据，存储到program_intregs和program_fpregs中
      - 调用alloc_memrange，分配所有内存区域
      - 调用setFirstload，恢复所有first load的数据
      - 调用read_ckptsyscall，读取所有的系统调用信息
      - 判断runningInfo.totalcallnum == 0，如果是，则此时就需要将退出指令替换为JmpTempReg(3)指令；否则交由takeOverSyscall函数在最后一次系统调用处理完后进行替换
      - 将npc保存到临时寄存器2中，用于跳转使用： SetTempReg(npc, 2)
      - 将takeOverAddr保存到临时寄存器3中，即JmpTempReg(3)指令将会跳转到takeOverAddr，完成ecall到takeOverSyscall函数的跳转
      - 将加载器的寄存器状态保存到OldIntRegAddr中（runninginfo.oldregs）
      - 根据ckpt中记录的warmup和原本片段长度信息，设置最大指令数和预热指令数；如果ckpt没有该信息，则预热指令数为simNum的5%，最大指令数为剩余的指令数
      - 恢复ckpt起始位置处的寄存器状态
      - 利用跳转到临时寄存器的指令JmpTempReg(2)，跳转到入口地址，开始执行

5. takeover_syscall.cpp: 接管和复现系统调用的影响
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
