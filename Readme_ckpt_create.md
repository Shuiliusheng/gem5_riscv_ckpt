### 处理gem5生成的运行时log文件，生成ckpt

1. 主要工作
    - 确定[start, exitInsts]中的start处的起始信息，访存信息，系统调用信息，使用的代码区域信息
    - 确定[start, exitInsts]的区域
    - 整理起始点信息
    - 整理系统调用信息
    - 整理访存信息，获取first load的信息
    - 根据访存信息和系统调用信息，确定[start, exitInsts]过程中使用的内存区域范围
    - 创建ckpt文件


3. 整理起始点信息: 根据type == int_regs中的信息获取以下信息
    - 记录start处的模拟指令数
    - 记录此时指令的pc和下一条指令的npc
    - 记录整数和浮点数寄存器的内容


4. 整理系统调用信息: 根据 type == syscall* 的line中收集以下信息
    - 系统调用信息必须包含的: 指令地址，系统调用号，参数， 返回值
    - 系统调用发生用户内存数据修改时的信息 


5. 整理访存信息，获取first load的信息
    - first load：当前load指令所访问的内存数据之前没有其他store写过，也没有被之前的load指令加载过
    - 使用两个set来寻找first load的信息
        - set prestore: 记录之前被store的地址
        - set preload: 记录之前被load的地址
        - 从开始遍历访存序列，将访存地址按照访存大小进行展开，使得访存单位变为byte
        - 如果load的某个地址不在prestore和preload中，则为first load，需要记录到loadinfo中
        - 将地址信息分别记录prestore和preload中
    - 将loadinfo中的信息，按照地址顺序排序，然后进行合并，使得load的数据单位从8->4->2->1的优先级进行组合


6. 根据访存信息和系统调用信息，确定[start, exitInsts]过程中使用的内存区域范围
    - 根据meminfo中的信息，将所有load和store的地址按照访存大小进行展开，使得访存单位变为byte，然后将这些地址放入addrset集合中
    - 根据syscallinfos中的信息，将系统调用所访问的内存区域的地址放入addrset中
    - 对addrset进行排序，遍历，以4KB为单位，进行第一次合并，得到以4KB对齐的起始地址+4KB的区域
    - 将相邻的4KB区域进行合并，得到最终的memrange信息


7. 创建ckpt文件
    - 将textinfo, exitinfo, startinfo, memrangeinfo, loadinfo, syscallinfo写入文件中，得到ckpt文件
    - 文件1: ckpt_{startplace}.info，记录textinfo, exitinfo, startinfo, memrangeinfo, loadinfo
        --------------------------------------
        | textinfo num              | 8      |
        --------------------------------------
        | textinfos(addr, size)     | num*16 |
        --------------------------------------
        | sim start place           | 8      |
        --------------------------------------
        | sim inst num              | 8      |
        --------------------------------------
        | exit inst pc              | 8      |
        --------------------------------------
        | start npc                 | 8      |
        --------------------------------------
        | start int regs            | 8*32   |
        --------------------------------------
        | start ip regs             | 8*32   |
        --------------------------------------
        | memrange num              | 8      |
        --------------------------------------
        | memrange info(addr, size) | num*16 |
        --------------------------------------
        | first load num            | 8      |
        --------------------------------------
        | load (addr, size, data)   | num*24 |
        --------------------------------------
        --------------------------------------
        | syscall info num          | 8      |
        --------------------------------------
        | pc                        | 8      |
        | num                       | 8      |
        | p0                        | 8      |
        | p1                        | 8      |
        | p2                        | 8      |
        | hasret                    | 8      |
        | ret                       | 8      |
        | bufaddr                   | 8      |
        | data_offset (file offset) | 8      |
        | data size                 | 8      |
        --------------------------------------
        | all syscall data information       |
        --------------------------------------