### 在原本readckpt的基础上增加和修改的内容
1. define.h: 定义了各种写入/读取/控制硬件寄存器和计数器的接口
  - GetInformation(tag, eaddr, mevent, nevent, esel, mpriv)：获取各个硬件寄存器的内容，用于调试使用
  - GetExitPC(exitpc): 获取采样发生时的指令地址
  - GetSampleHappen(shap): 获取当前sampleHappen寄存器的内容，用于判断当前是否发生了采样事件
  - SetSampleHappen(shap): 设置sampleHappen寄存器
  - SetCounterLevel(priv): 设置计数器统计事件的级别, "0" "1" "3"
  - SetPfcEnable(n): 设置pfcEnable寄存器，用于控制计数器工作与否
  - SetTempReg(value, n): 设置第n个临时寄存器为value
  - SetSampleBaseInfo(ptag, sampleFuncAddr): 设置procTag和sampleFuncAddr硬件寄存器
    - 当使用ptrace捕获采样引发的系统调用时，需要将sampleFuncAddr设置为0
  - SetSampleCtrlReg(maxevent, warmupinst, eventsel): 设置采样的事件选择，事件次数和预热的指令数
  - Load_Basic_Regs() / Save_Basic_Regs()
    - 将sampleFunc所需要的一些必要寄存器保存和恢复：sp, s0, gp, tp, ra
    - Load_Basic_Regs在initStart初始化时保存，并且s0和sp单独设置在一块开辟的空间，作为采样函数的栈空间使用
    - Save_Basic_Regs在采样函数开始时调用，恢复必要的五个寄存器
    - 开辟的栈空间为全局变量：uint64_t tempStackMem[4096]
    - tempStackMem的基址通过la t0, tempStackMem指令获得。为了避免la转换为gp+imm的形式，为此在tempStackMem之前定义一块空间，使得imm超过4KB，从而无法使用gp+imm
    - tempStackMem的初始32*8个字节用于保存上下文的所有32个整数寄存器，接下来5个用于保存sampleFunc函数所需要的五个寄存器
  - Save_ALLIntRegs() / Load_ALLIntRegs()
    - 用于保存和恢复上下文中所有32个整数寄存器（没有保存浮点寄存器，因此要求没有采样函数不使用浮点寄存器，否则需要保存）
    - 为了使用寄存器作为load和store指令的基址寄存器，同时避免修改寄存器的内容，因此首先将a0寄存器的内容保存在临时寄存器0中
    - 保存的地址为tempStackMem的初始32*8个字节，地址由la a0, tempStackMem指令获取
    - Save_ALLIntRegs保存所有寄存器之后，将临时寄存器0中存储的a0值，保存到对应内存中
    - Load_ALLIntRegs则不需要额外的操作，但是要将a0寄存器最后一个恢复
    - Save_ALLIntRegs在sampleFunc函数的入口初始出被调用
    - Load_ALLIntRegs在sampleFunc函数结束时调用，之后则使用URet指令跳转
  - RESET_COUNTER: 将所有计数器设置为0
  - GetCounter(basereg, dstreg, start, n): 读取计数器并存储到指定的地址
  - ReadCounter8(base, start): 读取从start开始的8个硬件计数器到base指定的地址中
  - ReadCounter16(base, start): 读取从start开始的16个硬件计数器到base指定的地址中

2. ctrl.h: 提供的一些c函数，用于完成对maxinst和warmup，以及exitfuc入口地址的设置
 - 全局变量定义
   ```c
      uint64_t placefolds[1024];  //用于避免la转换为gp+imm
      uint64_t tempStackMem[4096];//采样函数使用的栈空间
      uint64_t hpcounters[64];    //硬件计数器读取的内容
      uint64_t procTag=0x1234567; //进程tag
      //采样的控制
      uint64_t maxevent=10000, warmupinst=1000, eventsel=0, maxperiod=0;
      uint64_t sampleHapTimes=0;  //采样发生的次数
      extern uint64_t __sampleFucAddr;

      //用于统计全局时间和指令数使用
      uint64_t startcycle = 0, endcycle = 0;
      uint64_t startinst = 0, endinst = 0;
   ```
 - 函数定义
   - void read_counters(): 函数退出时读取硬件计数器的值
   - void based_sample_func()
     - asm volatile("__sampleFucAddr: ")使用该方法获取采样函数的入口地址
     - Save_ALLIntRegs()：保存上下文
     - Load_Basic_Regs()：恢复一些必要的寄存器
     - GetExitPC(exitpc)：获取采样发生时的指令地址，用于回到程序继续执行
     - SetTempReg(exitpc, 0)：将该地址写入临时寄存器0，用于之后利用URet跳转
     - sample_func(sampleHapTimes, exitpc)：实际采样处理函数
       - 需要注意由于printf函数会使用锁，因此为了避免产生死锁，我们使用write函数完成信息的输出和保存
     - SetPfcEnable(1);
     - SetSampleBaseInfo(procTag, &__sampleFucAddr);
     - SetSampleCtrlReg(maxevent, warmupinst, eventsel);
     - RESET_COUNTER; 重新设置采样参数
     - Load_ALLIntRegs()：恢复寄存器内容
     - JmpTempReg(0)：根据临时寄存器0中的地址发生跳转，回到程序继续执行

   void init_start(unsigned long long max_inst, unsigned long long warmup_inst);
     - Save_Basic_Regs保存采样函数需要使用的寄存器
      - sp单独设置为&tempStackMem[2048]
      - s0单独设置为&tempStackMem[2560]
     - 设置计数器统计级别，并使能计数器
       - 设置procTag和sampleFuncAddr
         - sampleFuncAddr并不是编译后samplefunc函数的第一条指令，而是Save_ALLIntRegs的第一条指令，即开始就要保存上下文中的所有寄存器
       - 设置采样的控制 SetSampleCtrlReg(maxevent, warmupinst, eventsel)
       - 清空所有计数器
     - 在read_ckptinfo函数最终要恢复寄存器之前，调用该函数，设置maxinst和warmup信息
    