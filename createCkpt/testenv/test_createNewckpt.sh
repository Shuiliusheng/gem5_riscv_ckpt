#!/bin/bash

# if [[ $# < 2 ]]; then
#     echo "parameters are not enough!"
#     echo "./run.sh settings bench ckptfile"
#     exit
# fi


gem5_dir=/home/gem5_src/gem5
readckpt=/home/gem5_src/gem5/testenv/readckpt/readckpt.riscv
# settingfile=$1
# bench=$2
# ckptfile=$3
settingfile=/home/gem5_src/gem5/testenv/setting_nckpt
bench=/home/gem5_src/gem5/testenv/gcc.riscv
ckptfile=/home/gem5_src/gem5/testenv/gcc_nckpt_15000000000_len_6000000_warmup_1000000.info
options="$ckptfile $bench"


if [[ ! -e $bench ]]; then
    echo $bench "is not exist!"
    exit
fi

# create setting file
echo "mmapend: 0x350000000" | tee $settingfile
echo "stacktop: 0x6a0000000" | tee -a $settingfile
echo "ckptprefix: /home/gem5_src/gem5/testenv/newckpt/gcc" | tee -a $settingfile
#create two checkpoint file start with the 1000000th inst
#each contain 2M insts and 500K insts for warmup
echo "ckptctrl: 1000000 2000000 500000 2" | tee -a $settingfile
echo "readckpt: $ckptfile"  | tee -a $settingfile 


# temp=`echo $ckptfile | sed 's#\/#\\\/#g'`
# temp=`echo $temp | sed 's#\-#\\\-#g'`
# echo sed -i "s/readckpt.*/readckpt: $temp/g" $settingfile
# sed -i "s/readckpt.*/readckpt: $temp /g" $settingfile


$gem5_dir/build/RISCV/gem5.fast $gem5_dir/configs/example/se.py --cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB --ckptsetting=$settingfile -c $readckpt --options="$options"
