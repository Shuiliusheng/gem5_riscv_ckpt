#### create cvp traces with gem5 （riscv）
#### 使用方法
shell/genTraces.sh
- tracesetting=settingsfile
  - 指定生成traces的配置文件
- settingsfile
  - maxpc: 0x4000000 
  - minpc: 0x200000 
    - 要统计的指令pc范围
  - tracectrl: 0 20000000 1
  - tracectrl: 100000000 20000000 1
    - 要通知的区域范围
    - tracectrl: start length num
    - 从多少条指令开始 统计多少条指令 一共多少个连续间隔
  - ckptstart: 12400000000 
    - 使用ckpt时的参数，指明当前ckpt的起始指令是从哪里开始的
  - logprefix: astar  
    - 输出文件的前缀


#### 修改的代码
1. configs增加tracesetting的参数设置
    - common/Options.py中增加参数
    ```python
        def addNoISAOptions(parser):
            parser.add_argument("--tracesetting", default="", help="The File for Setting Trace Create.")
    ```
    - example/se.py增加参数设定
    ```python 
        def get_processes(args):
            for wrkld in workloads:
                process = Process(pid = 100 + idx)
                if '--tracesetting' in str(sys.argv):
                    process.tracesetting = args.tracesetting
    ```

2. 增加traces support的头文件和源文件
    - sim/trace_collect.hh
    - sim/trace_collect.cc

3. sim/SConscript中增加新的cc文件
    - Source('trace_collect.cc')

4. sim/Process.py中的修改，增加traces的param参数
    - 增加tracesetting的参数
    ```python
        tracesetting = Param.String('', 'simulation the mmap addr end place')
    ```

5. sim/process.cc中的修改，以初始化traces设置的读取
    - 增加头文件：#include "sim/trace_collect.hh"
    - Process::Process函数中
    ```c
        //before
        if (loader::debugSymbolTable.empty())
            loader::debugSymbolTable = objFile->symtab();

        //after
        if (loader::debugSymbolTable.empty())
            loader::debugSymbolTable = objFile->symtab();

        init_trace_settings(params.tracesetting.c_str());
        cout <<"params.tracesetting:"<<params.tracesetting<<endl;
    ```

6. cpu/simple/atomic.cc中修改，以记录指令执行时信息
    - 增加头文件：#include "sim/trace_collect.hh"
    - readMem函数的修改
    ```c
        //before
        if (size_left == 0) {
            if (req->isLockedRMW() && fault == NoFault) {
                assert(!locked);
                locked = true;
            }
            xxxx
        }

        //after 
        if (size_left == 0) {
            if (req->isLockedRMW() && fault == NoFault) {
                assert(!locked);
                locked = true;
            }
            //for creating traces
            if(traceOpen) {
                record_meminfo(thread->pcState().pc(), addr);
            }
            xxxx
        }
    ```

    - writeMem函数的修改
    ```c
        //before
        const RequestPtr &req = data_write_req;

        if (traceData)
            traceData->setMem(addr, size, flags);


        //after
        //for creating traces
        if(traceOpen) {
            record_meminfo(thread->pcState().pc(), addr);
        }

        const RequestPtr &req = data_write_req;

        if (traceData)
            traceData->setMem(addr, size, flags);

    ```

    - amoMem函数的修改
    ```c
        //before
        //If there's a fault and we're not doing prefetch, return it
        return fault;

        //after
        //for creating traces
        if(traceOpen) {
            record_meminfo(thread->pcState().pc(), addr);
        }
        return fault;

    ```

    - tick()的修改
    ```c
        //before
        if (fault == NoFault) {
            countInst();
            ppCommit->notify(std::make_pair(thread, curStaticInst));
            xxxx
        }

        //after
        if (fault == NoFault) {
            countInst();
            ppCommit->notify(std::make_pair(thread, curStaticInst));
            
            //for creating traces
            if(traceOpen) {
                record_traces(thread, curStaticInst);
            }
            xxxx
        }

    ```