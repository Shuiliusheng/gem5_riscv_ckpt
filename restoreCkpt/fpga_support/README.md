### 扩展boom的源码 ####
1. 修改rocket-chip/src/main/scala/subsystem/Configs.scala文件, 扩展默认的mem_size
   - 原本memsize = 0x10000000 -> 0x40000000
   - 通过扩展memsize, 以使得模拟过程中可用的栈空间和mmap的空间变大
   - (创建ckpt时是缩小stack和mmap的空间, 从而使得重新恢复ckpt时能够使用mmap直接将stack和其他区域进行映射)

2. 修改boom v3的源码, 以支持临时寄存器的读写和跳转
   - 增加四个临时寄存器: 将原本32个逻辑寄存器扩展到36个, 其余不变
   - 单独处理读/写/跳转临时寄存器的指令的译码逻辑, 将ldst/lrs2/lrs1字段单独设置为临时寄存器(r32, r33, r34, r35)
   