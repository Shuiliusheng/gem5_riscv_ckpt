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

2. sim/SConscript中增加新的cc文件
    - Source('trace_collect.cc')

2. sim/Process.py中的修改，增加traces的param参数
    - 增加tracesetting的参数
    ```python
        tracesetting = Param.String('', 'simulation the mmap addr end place')
    ```

2. sim/process.cc中的修改，以初始化traces设置的读取
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

3. cpu/simple/atomic.cc中修改，以记录指令执行时信息
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