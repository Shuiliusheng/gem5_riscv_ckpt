## 用于完成ckpt文件相关的一些工作
1. compckpt: 压缩ckpt文件中的firstload信息
    - 原始first load信息的格式：(addr(8B) + data(8B)) * loadnum
    - 压缩：
        - 将数据按照4KB的范围进行组织，使用map表来记录非零数据的位置。addr(8B) + size(8B) + datamap(4096/64) + datas
        - 利用fastlz中提供的压缩函数将多个4KB信息的数据压缩，然后存储
    - 功能：
        - 输入的ckpt文件可以是未压缩的或者是不同压缩级别后的文件
        - 输出的压缩级别由参数指定，输出也可以是不进行任何压缩的ckpt文件
2. showckpt: 读取ckpt文件中的信息
3. rewriteSysInfo: 将ckpt中系统调用的信息重新替换为idx + diff syscall information的格式