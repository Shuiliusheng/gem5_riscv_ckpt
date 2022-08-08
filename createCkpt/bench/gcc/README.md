### 介绍
1. createCkpt.sh中设置了gcc可执行文件的目录和输入参数，以及Gem5的参数和目录
2. createCkpt.sh中选择使用的settingfile是settings_pk文件
   ```c
    //mmapend和stacktop设定, brkpoint采用默认的0x1400000
    mmapend: 0x30000000
    stacktop: 0x74000000
    //生成ckpt文件的目录为ckpt文件夹，前缀为gcc
    ckptprefix: ./ckpt/gcc
    //从100000000指令开始，间隔1亿条生成一个checkpoint文件，共生成2个
    //每个checkpoint文件预热两千万条指令
    //checkpoint1: 从8千万条指令开始，记录2千万作为预热指令，从1亿开始记录1亿条指令的信息
    //checkpoint2: 从1亿8千万条指令开始，记录2千万作为预热指令，从2亿开始记录1亿条指令的信息
    ckptctrl: 100000000 100000000 20000000 2

   ```