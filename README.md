Checkpoint Create and Restore (RISC-V)

1. 整体思路

   - 保存执行时信息：例如创建10000(start)-20000(end)条指令之间的checkpoint

     - 记录start处的：PC，NPC，寄存器值
     - 记录[start, end]之间程序的访存行为：(load, store, atomic) * (addr, size, data) 
     - 记录[start, end]之间程序触发系统调用时：函数参数，返回值，需要写回到用户空间的数据和PC

   - 生成checkpoint：

     - 确定start处需要恢复的数据：NPC，RegInfo
     - 根据记录的访存序列确定[start, end]之间程序所使用的内存区域：memrange
     - 根据记录的访存序列确定[start, end]之间满足程序正确运行的内存状态：firstload
     - 根据记录的系统调用行为，确定发生系统调用时：系统调用号&参数&PC，需要更改的内存状态（回写的数据），需要更改的寄存器值（返回值）：syscallinfo

   - 恢复checkpoint：使用启动程序恢复ckpt，加载执行程序，接管系统调用

     - mmap映射一段固定位置的可读写区域，用于存储中间数据和固定信息，包括接管程序执行时的系统调用信息，当前系统调用发生的次数，和系统调用时需要保存的寄存器状态以及启动程序的寄存器状态（用于满足正常执行系统调用）

       ```c
       typedef struct{
           uint64_t syscall_info_addr;
           uint64_t nowcallnum;
           uint64_t totalcallnum;
           uint64_t intregs[32];
           uint64_t fpregs[32];
           uint64_t oldregs[32];
       }RunningInfo;
       uint64_t alloc_vaddr = (uint64_t)mmap((void*)RunningInfoAddr, 4096*2, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
       ```

     - 映射程序的代码段到内存中（事先需要在编译程序时，将程序的代码段和数据段放置在不影响启动程序的地方），替换代码段中的ecall为jmpRtemp指令

     - 映射memrange

     - 恢复firstload数据

     - 加载syscallinfo信息，将信息的起始地址放入RunningInfoAddr.syscall_info_addr

     - 将NPC写入临时寄存器，同时将接管系统调用的程序入口地址写入临时寄存器

     - 加载RegInfo信息到寄存器

     - jump rtemp，开始执行程序

     - 遇到jmpRtemp（原本的ecall） -> 跳转到接管程序 -> 保存寄存器状态 -> 恢复支撑执行的启动程序寄存器状态 -> 读取syscallinfo -> 比对正确性 -> 恢复内存数据 -> 确定返回值 -> 恢复寄存器状态 -> 利用JmpRTemp跳转到NPC

     - 终止：发生系统调用不匹配（不够好），硬件发现指令达到end，主动跳转到接管程序，直接终止

2. 硬件辅助：增加若干临时寄存器，用于在恢复ckpt时使用

   - 读取临时寄存器：

     - 用于读取临时寄存器中的值到寄存器中，主要用于在保存完运行时状态的结尾恢复某个寄存器的值，搭配write rtemp使用

     ```assembly
     # add x0, rd, rtemp
     # REG[rd] = REG[rtemp]
     add x0, a1, x7  #read rtemp7 to reg a1
     ```

   - 写临时寄存器：

     - 用于在保存运行时状态时，暂存某个寄存器的值，搭配read rtemp使用
     - 用于设置某些固定的跳转地址，搭配jmp rtemp使用

     ```c
     // sub x0, rs1, rtemp
     // REG[rtemp] = REG[rs1]
     #define WriteTemp(num, val) asm volatile( \
         "mv t0, %[data] \n\t"   \
         "sub x0, t0, x" num " \n\t"   \
         "fence \n\t"   \
         :  \
         :[data]"r"(val)  \
     );
     ```

   - 根据临时寄存器跳转

     - 用于跳转到需要执行程序的起始地址（使用pc+imm无法跳到足够远的地方）
     - 用于接管系统调用时，跳转到固定的系统调用处理程序入口（提前将执行程序的ecall替换为jmptemp指令）

     ```c
     // or x0, x0, rtemp
     // jump to REG[rtemp]
     #define JmpTemp(num) asm volatile( \
         "or x0, x0, x" num " \n\t"   \
     );
     ```

3. 软件实现

   - 模拟器支持

     - 记录ckpt信息：根据设置的ckpt的间隔指令数，统计访存信息，系统调用信息和寄存器状态

       ```json
       {"type": "mem_read", "pc": "0x200786", "addr": "0x7ffffdf8", "size": "0x8", "data": "0x1"}
       {"type": "mem_read", "pc": "0x20078c", "addr": "0x7ffffe00", "size": "0x8", "data": "0x7fffff38"}
       {"type": "mem_write", "pc": "0x200792", "addr": "0x26ab60", "size": "0x4", "data": "0x0"}
       {"type": "mem_write", "pc": "0x200796", "addr": "0x26ad40", "size": "0x8", "data": "0x7fffff48"}
       {"type":"syscall enter", "pc": "0x21b354", "sysnum": "0x50", "param": [ "0x1", "0x7ffff670", "0x7ffff670", "0x0", "0x5e8" ]}
       {"type":"syscall info", "info": "setdata", "pc": "0x21b354", "buf": "0x7ffff670", "bytes": "0x80", "ret": "0x0", "data": [ "0xa","0x0","0x0","0x0","0x0","0x0","0x0","0x0","0x3","0x0","0x0"]}
       {"type":"syscall return", "sysnum": "0x50", "sysname":"(fstat(0, 0x7ffff670))", "pc": "0x21b354", "res":"has ret", "val": "0x0"}
       {"type": "int_regs", "inst_num": "6000", "inst_pc": "0x21070e", "npc": "0x210710", "data": [ "0x0", "0x2118e4", "0x7ffff700", "0x26b1b0", "0x26d710", "0xa", "0x7fffffde", "0x1", "0x2", "0x264000", "0x269218", "0x26e2e0", "0x2", "0x26e2e0", "0x1000", "0xfffffffffbad2a84", "0x64", "0xffffffffffffffff", "0x263a88", "0x263d28", "0x1", "0x263a88", "0x248ea0", "0x269218", "0x20", "0x249b58", "0x1", "0x248ea2", "0x0", "0x1", "0x0", "0x20" ]}
       ```

     - 支持恢复ckpt：增加一些额外的指令，支持临时寄存器的读写跳转，支持统计程序执行的指令数（到达指令数后，结束执行）

       ​

   - restore软件支持