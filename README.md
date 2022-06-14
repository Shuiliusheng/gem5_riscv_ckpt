Checkpoint Create and Restore (RISC-V)

1. 整体思路

   - 保存执行时信息：例如创建10000(start)-20000(end)条指令之间的checkpoint

     - 记录start处的：PC，NPC，寄存器值
     - 记录[start, end]之间程序的访存行为：$(load, store, atomic) * (addr, size, data)$ 
     - 记录[start, end]之间程序触发系统调用时：函数参数，返回值，需要写回到用户空间的数据和PC

   - 生成checkpoint：

     - 确定start处需要恢复的数据：NPC，RegInfo
     - 为了保证start处开始模拟后，可以正确停止模拟，因此需要确定退出条件，目前方法如下：
        - 统计从end之前发生的最后一次系统调用开始$[end, exitInst)$之间第一次出现的指令，将其作为exitInst
        - 记录exitInst的信息：当前指令数，当前指令pc（需要注意一点，需要判断该pc所处位置的四个字节都是第一次在[end, exitInst)中出现）
        - 在restore过程中，如果$[start, exitInst]$之间没有系统调用，则在启动程序之间，将exitInst替换为jmpRtemp，跳转到syscallTakeover程序，然后exit
        - 在restore过程中，如果存在系统调用，则在syscallTakeover中检测到最后一次系统调用时，将exitInst替换为jmpRtemp，然后执行到该处之后再退出
     - 根据记录的访存序列确定$[start, exitInst]$之间程序所使用的内存区域：memrange
        - 根据load/store/atomic访存地址和宽度确定范围
        - 根据系统调用时所访问的存储区确定范围
     - 根据记录的访存序列确定$[start, exitInst]$之间满足程序正确运行的内存状态：firstload
     - 根据记录的系统调用行为，确定发生系统调用时：系统调用号&参数&PC，需要更改的内存状态（回写的数据），需要更改的寄存器值（返回值）：syscallinfo
     - 实际模拟的指令数：$exitInst-start$

   - 恢复checkpoint：使用启动程序恢复ckpt，加载执行程序，接管系统调用

     - mmap映射一段固定位置的可读写区域，用于存储中间数据和固定信息，包括接管程序执行时的退出信息，系统调用信息，当前系统调用发生的次数，和系统调用时需要保存的寄存器状态以及启动程序的寄存器状态（用于满足正常执行系统调用）

       ```c
       typedef struct{
          uint64_t exitpc;
          uint64_t exit_cause;
          uint64_t syscall_info_addr;
          uint64_t nowcallnum;
          uint64_t totalcallnum;
          uint64_t intregs[32];
          uint64_t fpregs[32];
          uint64_t oldregs[32];
       }RunningInfo;
       uint64_t alloc_vaddr = (uint64_t)mmap((void*)RunningInfoAddr, 4096*2, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
       ```

     - 映射程序的代码段到内存中（事先需要在编译程序时，将程序的代码段和数据段放置在不影响启动程序的地方）
      - 根据执行过程中统计的信息，确定程序执行所需要最大的代码段范围（多段），加载代码
      - 替换每段被加载的代码中的ecall为jmpRtemp指令

     - 映射memrange

     - 恢复firstload数据

     - 加载syscallinfo信息，将信息的起始地址放入RunningInfoAddr.syscall_info_addr

     - 将NPC写入临时寄存器，同时将接管系统调用的程序入口地址写入临时寄存器

     - 加载RegInfo信息到寄存器

     - jump rtemp，开始执行程序

     - 遇到jmpRtemp（原本的ecall） -> 跳转到接管程序 -> 保存寄存器状态 -> 恢复支撑执行的启动程序寄存器状态 -> 读取syscallinfo -> 比对正确性 -> 恢复内存数据 -> 确定返回值 -> 恢复寄存器状态 -> 利用JmpRTemp跳转到NPC

     - 终止：发生系统调用不匹配（不够好），硬件发现指令达到end，主动跳转到接管程序，直接终止

2. 硬件辅助：增加四个临时寄存器，用于在恢复ckpt时使用

   - 读取临时寄存器：

     - 用于读取临时寄存器中的值到寄存器中，主要用于在保存完运行时状态的结尾恢复某个寄存器的值，搭配write rtemp使用

     ```c
     // ori x0, rs1, #4-7   # REG[rs1] = REG[0, 1, 2, 3]
     #define ReadRTemp(dstreg, rtempnum) "ori x0, " dstreg ", 4+" rtempnum " \n\t"
     ```

   - 写临时寄存器：

     - 用于在保存运行时状态时，暂存某个寄存器的值，搭配read rtemp使用
     - 用于设置某些固定的跳转地址，搭配jmp rtemp使用

     ```c
     // ori x0, rs1, #8-11  # REG[0, 1, 2, 3] = REG[rs1]
     #define WriteRTemp(srcreg, rtempnum) "ori x0, " srcreg ", 8+" rtempnum " \n\t"
     ```

   - 根据临时寄存器跳转

     - 用于跳转到需要执行程序的起始地址（使用pc+imm无法跳到足够远的地方）
     - 用于接管系统调用时，跳转到固定的系统调用处理程序入口（提前将执行程序的ecall替换为jmptemp指令）

     ```c
     // ori x0, x0, #12-15  # jump to REG[0, 1, 2, 3]
     #define JmpRTemp(rtempnum) "ori x0, x0, 12+" rtempnum " \n\t"
     ```

3. 软件实现

   - 模拟器支持
   - 模拟器支持直接生成checkpoint文件，并且支持从文件中读取生成ckpt的控制信息
    - 执行参数：build/RISCV/gem5.opt --debug-flag=CreateCkpt --debug-file=temp ./configs/example/se.py --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=settings -c bwaves.riscv
    - setting文件格式
      - mmapend/stacktop指定运行时的一些内存地址信息
      - ckptprefix: 指定输出结果的目录和文件名前缀
      - ckptctrl: (startckpt interval warmupnum times)
    ```c
      mmapend: 261993005056
      stacktop: 270582939648
      ckptprefix: ./res/bwaves
      ckptctrl: 10000 10000 0 4
    ```
