### create ckpt with gem5
1. 执行和支持的log信息类型
    - 参数：--debug-flag=CreateCkpt
        - 利用debug-flag来确定是否需要创建checkpoint
    - 参数：--debug-file=logfile , 用于控制log文件的位置和名称，原本gem5的参数
    - 参数：--ckptsettings=settingsfile 
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

2. 运行示例
    ```shell
        build/RISCV/gem5.opt --debug-flag=CreateCkpt --debug-file=temp ./configs/example/se.py --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=settings -c bwaves.riscv
    ```