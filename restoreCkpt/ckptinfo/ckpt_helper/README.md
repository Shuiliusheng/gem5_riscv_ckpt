## 用于完成ckpt文件相关的一些工作
1. compckpt: 压缩ckpt文件中的firstload信息
    - 原始first load信息的格式：(addr(8B) + data(8B)) * loadnum
    - 压缩：
        - 将数据按照4KB的范围进行组织，使用map表来记录非零数据的位置。addr(8B) + size(8B) + datamap(4096/64) + datas
        - 利用fastlz中提供的压缩函数将多个4KB信息的数据压缩，然后存储
2. uncompckpt: 将压缩后的ckpt文件进行解压（提前解压会增加大小，但是读取时间将会变快）
    - 目前解压是仅将fastlz压缩的数据解压
3. showckpt: 读取ckpt文件中的信息
4. rewriteSysInfo: 将ckpt中系统调用的信息重新替换为idx + diff syscall information的格式