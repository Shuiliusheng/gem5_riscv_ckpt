### 读取生成的ckpt文件，加载测试程序运行（不需要硬件支持）
1. ckptinfo.h: 唯一的头文件，用于定义一些struct, 汇编代码, 函数, extern变量
    - 常量定义
    ```c
        //riscv中jal最大的跳转距离
        #define MaxJALOffset 0xFFFF0
    ```
    - 结构体定义
    ```c
        typedef struct{
            uint64_t jalr_target;   //第一条jalr指令所需要的目标地址
            uint64_t jal_addr;  //最后一条jal指令所需要放置的地址和指令
            uint32_t jal_inst;  //该jal指令将完成最后的跳转工作
        }JmpRepInfo; //从readckpt跳转到program地址所需要的信息

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
            uint32_t exitJmpInst;   //退出指令应该被替换为的jal指令内容
            JmpRepInfo *sysJmpinfos;//每个从readckpt跳转到program地址所需要的信息

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
        //加载器向benchmark执行环境跳转的第一阶段：将a0设置为目标地址，利用jalr a0跳转到目标地址附近的空闲区域
        #define StartJump() asm volatile( \
            "la a0, tempMemory  \n\t"   \
            "ld a0, 0(a0)  \n\t"        \
            "jalr x0, 0(a0)  \n\t"      \
        ); 
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
        //获取ckpt中所有的内存区域信息
        void getRangeInfo(char filename[]);
        //根据中间节点和跳转需求，确定所有跳转需求的实际操作
        void processJmp(uint64_t npc);
        //根据JmpRepInfo，完成对跳转指令的设置
        void updateJmpInst(JmpRepInfo &info);
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
    - void alloc_memrange(FILE *p)
      - 从ckpt文件中读取所有的内存区域信息MemRangeInfo
      - 将所有的内存区域使用map函数进行映射
      - 由于之前代码段已经完成了映射，因此判断当前内存区域是否和代码段有部分重叠，如果有则需要单独处理和映射
      - （访问执行过程中访问了代码段的部分区域，如果不映射代码段所有区域，那就需要单独比较所有记录的代码区域）
    - void setFistLoad(FILE *p)
      - 从ckpt文件中读取所有的first load信息，并且将其恢复到内存中
    - void read_ckptinfo(char ckptinfo[])
      - 主要任务：读取ckpt文件，恢复所有执行环境，完成跳转转换，启动benchmark执行
      - 读取ckpt文件的头部，获取程序片段的起始指令数，片段指令数（到退出指令为止），设置的片段长度，warmup的指令数，退出指令的pc，退出的原因，片段的第一条指令pc地址
      - 从ckpt文件中获取整数和浮点寄存器的数据，存储到program_intregs和program_fpregs中
      - 调用alloc_memrange，分配所有内存区域
      - 调用setFirstload，恢复所有first load的数据
      - 调用read_ckptsyscall，读取所有的系统调用信息
      - 调用getRangeInfo和produceJmp，完成对所有跳转需求的转换
      - 判断runningInfo.totalcallnum == 0，如果是，则此时就需要将退出指令替换为jal指令(runningInfo.exitJmpInst)；否则交由takeOverSyscall函数在最后一次系统调用处理完后进行替换
      - 设置跳转到npc的跳转指令序列
        - updateJmpInst(runningInfo.sysJmpinfos[0]), sysJmpinfos[0]存储着跳转到ckpt第一条指令所需要的信息
      - 将加载器的寄存器状态保存到ReadCkptIntRegs
      - 恢复ckpt起点处的寄存器状态
      - 使用StartJump开始跳转到npc开始执行

5. user_jmp.cpp: 完成寻找能够满足所有跳转需求的地址的跳转指令
    - 全局变量的定义
        ```c++
            //用于记录program代码段中所有空闲内存区域的信息
            vector<MemRangeInfo> freeRanges;
            //记录所有跳转需求的信息
            typedef struct {
                uint64_t label; //用于排序
                uint64_t pc;    //跳转的目标地址或者发生地址
                uint64_t midpoint;//对应的空闲区域
            }PCs;
            //vector<PCs> &pcs
        ```
    - 一些基础函数定义
        ```c++
            //根据基址base和目标地址target创建riscv的jal指令，返回是32位的指令
            uint32_t createJAL(uint64_t base, uint64_t target);
            //根据源寄存器生成jalr x0, 0(srcreg)指令
            uint32_t createJALR(uint32_t srcreg)
            //根据源寄存器，目的寄存器和偏移生成ld指令
            uint32_t createLD(uint32_t dstreg, uint32_t basereg, uint32_t offset);
            //根据目的寄存器和32位立即数生成lui+addi指令，以仅使用dstreg来完成32位立即数的加载
            void createImm32(uint32_t dstreg, uint32_t imm32, uint32_t &inst1, uint32_t &inst2);
        ```
    - void getRangeInfo(char filename[])
      - 读取ckpt文件，从中获取代码段中需要用的内存区域
      - 根据这些区域，找到代码段中所有尚未分配的空闲内存区域，用于之后寻找中间点
  
    - void findMidPoints(vector<PCs> &pcs)
      - 为pcs中记录的每个pc在尚未使用的空闲区域中寻找一块区域，使其满足该区域和所记录的pc之间能够相互使用jal指令完成跳转
      - 所有中间点会记录在对应的PCs中的midpoint中
  
    - void createInsts_jmp2loader(vector<uint64_t> &pcs, uint32_t memaddr)
      - 目标：将pcs中所有需要从program跳转到readckpt代码段的跳转需求实现（memaddr为&takeOverAddr）
      - 对象：系统调用ecall指令和退出指令，前者直接完成指令替换，后者在特定实际完成指令替换
      - 具体设计：以ecall为例
        - 将ecall替换为jal指令，使其跳转到之前寻找到的midpoint的地址出
        - 利用createImm32生成两条指令，放置在midpoint起始处，将a0寄存器设置为指定数（&takeOverAddr）
        - 利用createLD生成ld指令，紧接着放置在后面，用于将takeOverAddr的内容读取到a0中
        - 利用createJALR生成jalr x0, 0(a0)指令，放置在最后，实现跳转到takeOverFunc地址
        - 从midpoint起始地址开始，共放置四条指令，16 Bytes
  
    - void createInsts_jmp2prog(vector<PCs> &pcs, uint32_t memaddr)
      - 目标：实现从readckpt跳转到program的代码段，完成ckpt的启动和系统调用的返回
      - 对象：ckpt起始地址（pcs[0]）, 系统调用返回地址（pcs[i].pc+4）
      - 在恢复ckpt时，提前在midpoint区域的指令设置：
        - 在midpoint+32的地址处开始放置指令用于恢复a0寄存器，从而使用readckpt能够利用jalr x0, a0完成跳转
        - 依次放置的指令：
          - lui+addi设置a0为指定32位立即数（program_intregs[10]的地址）
          - ld a0,0(a0)完成a0的恢复
          - jal指令，用于跳转到startpc或者ecallpc+4（该指令在实际发生跳转时设置，因此该信息记录在JmpRepInfo中）
      - 实际发生跳转时readckpt中代码的设计：
        - 根据JmpRepInfo记录的jal指令的地址和内容，完成jal指令的设置
        - 将a0寄存器设置为所要跳转的目标地址相应的midpoint+32地址
        - 利用jalr指令完成跳转
        - 开始执行恢复a0的指令
        - 最后执行刚完成替换的jal指令，跳转到需要的地址
      - 每个目标地址所需要记录的信息JmpRepInfo
        - readckpt需要跳转的目标地址jalr_target：midpoint+32
        - readckpt需要完成的jal指令的替换信息：地址（midpoint+32+4*3） + 指令
  
    - void processJmp(uint64_t npc)
      - 对外的接口函数
      - 汇总所有需要跳转的目标地址到pcs变量中
      - 调用createInsts_jmp2loader，完成ecall的替换和退出指令的替换指令计算runningInfo.exitJmpInst
      - 调用createInsts_jmp2prog，记录每个跳转需求实际发生时所需要的信息
  
    - void updateJmpInst(JmpRepInfo &info)
      - 将info.jalr_target设置在一块指定的区域tempMemory[0]
      - 将info.jal_inst放置在info.jal_addr指定的位置
      - 之后将可以使用StartJump完成跳转
        - la a0, tempMemory 获取地址到a0中
        - ld a0, 0(a0), 从tempMemory[0]中获取info.jalr_target
        - jalr x0, 0(a0), 完成实际的跳转


6. takeover_syscall.cpp: 接管ecall，完成对内存的更改和返回值的设定
    - takeoverSyscall函数：用于接管系统调用，完成对内存的修改，设定返回值，监测运行是否正确，完成退出指令的替换
    - 工作流程：
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
      - step9: 判断是否已经是最后一个系统调用。如果是，则将退出指令所在的地址替换为跳转指令
        - *((uint32_t *)runningInfo.exitpc) = runningInfo.exitJmpInst
      - 更新返回到该系统调用下一条指令所需要的跳转指令
        - updateJmpInst(runningInfo.sysJmpinfos[runningInfo.nowcallnum])
      - 完成系统调用的处理，恢复benchmark的寄存器状态
      - startJump开始跳转回到系统调用的返回地址