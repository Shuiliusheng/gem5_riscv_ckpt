### 支持warmup和执行指定指令数的readckpt
- ctrl.h 和 define.h中提供了接口，用于设定warmup指令数和要运行的最大指令数
- 在执行到最大指令数时，会自动跳转到提供的退出函数入口exit_fuc，该函数能够可以自行增加代码
- 该功能需要硬件的支持，具体支持可见boom_stop的仓库
