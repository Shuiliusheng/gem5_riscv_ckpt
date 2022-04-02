#!/bin/bash

ARGC=$# 
if [[ "$ARGC" < 2 ]]
then
    echo "./xxx.sh arch exe "
    exit
fi

if [[ ! -e $2 ]]; then
    echo $2 " is not exist."
    exit
fi

#arm riscv mips alpha amd64
if [ $1 = "arm" -o $1 = "ARM" ]; then
    ISA="arm"
    gem5_arch=ARM/gem5.opt
elif [ $1 = "riscv" -o $1 = "RISCV" ]; then
    ISA="riscv"
    gem5_arch=RISCV/gem5.opt
elif [ $1 = "mips" -o $1 = "MIPS" ]; then
    ISA="mips"
    gem5_arch=MIPS/gem5.opt
elif [ $1 = "alpha" -o $1 = "ALPHA" ]; then
    ISA="alpha"
    gem5_arch=ALPHA/gem5.opt
else
    ISA="amd64"
    gem5_arch=X86/gem5.opt  
fi

# ************************** spec 2006 parameters ******************************
gem5_dir=/home/gem5_src/gem5
gem5_config=configs/example/se.py
gem5_output_dir=$gem5_dir/gem5_data

debug_flag="--debug-flags=ShowCMOVAssembly" #--debug-flags=ShowRunning
#debug_flag="" 

#--maxinsts=100000000 --fast-forward=10000000
run_args="--maxinsts=1000000000"
#o3 args: "--cpu-type=DerivO3CPU  --mem-size=8GB --l1d_size=64kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=4 --l2cache --l2_size=2MB --l2_assoc=16 "
#atomic args: "--cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB"
gem5_args="--cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB"
gem5_args=$gem5_args" "$run_args	
cmd=$2

# ******************************************************************************* #
curTime=$(date "+%y%m%d-%H%M%S")
gem5_output_dir=$gem5_output_dir/test-$ISA-$curTime
mkdir -p $gem5_output_dir
script_out=$gem5_output_dir/runscript.log

# ******************************************************************************** #
echo " " | tee $script_out
echo "******************************directory****************************" | tee -a $script_out
echo "gem5_dir: "$gem5_dir | tee -a $script_out
echo "gem5_output_dir: "$gem5_output_dir | tee -a $script_out
echo "" | tee -a $script_out

echo "******************************benchmark****************************" | tee -a $script_out
echo "executable: "$cmd | tee -a $script_out
echo "" | tee -a $script_out

echo "******************************gem5****************************" | tee -a $script_out
echo "arch:       "$gem5_dir/build/$gem5_arch | tee -a $script_out
echo "config:     "$gem5_dir/$gem5_config | tee -a $script_out
echo "args:       "$gem5_args | tee -a $script_out
echo "" | tee -a $script_out

echo "******************************simulation****************************" | tee -a $script_out
echo "start time:(year-mon-day hour-minute-second)"$(date "+%y-%m-%d %H-%M-%S")| tee -a $script_out


#************************************ run gem5 **************************************/
# --options="$args" --output=${benchmark_output}/$output --input=$input --errout=${benchmark_output}/$errout 
$gem5_dir/build/$gem5_arch \
    --outdir=$gem5_output_dir  $debug_flag \
    $gem5_dir/$gem5_config $gem5_args \
    --cmd=$cmd | tee -a $script_out
    
echo "end time:(year-mon-day hour-minute-second)"$(date "+%y-%m-%d %H-%M-%S")| tee -a $script_out
