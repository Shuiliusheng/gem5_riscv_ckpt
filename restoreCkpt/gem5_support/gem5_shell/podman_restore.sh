#!/bin/bash
local_gem5_dir=./gem5_riscv_ckpt
build_map="-v ${local_gem5_dir}/restoreCkpt/build_restore:/home/gem5_src/gem5/build"
src_map="-v ${local_gem5_dir}/restoreCkpt/gem5_restore:/home/gem5_src/gem5/src"
config_map="-v ${local_gem5_dir}/createCkpt/gem5_configs:/home/gem5_src/gem5/configs"
ckptinfo="-v ${local_gem5_dir}/restoreCkpt/ckptinfo:/home/gem5_src/gem5/ckptinfo"
local_dir_map="-v /home/cuihongwei/experiment/local_dir:/root/local_dir"

map=$build_map" "$src_map" "$config_map" "$ckptinfo

podman run -it --rm $map $local_dir_map  registry.cn-hangzhou.aliyuncs.com/ronglonely/gem5:3.0 /bin/bash