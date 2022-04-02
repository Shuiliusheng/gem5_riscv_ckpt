#!/bin/bash
local_gem5_dir=/home/cuihongwei/experiment/gem5_dir/gem5_riscv_ckpt
build_map="-v ${local_gem5_dir}/build_restore:/home/gem5_src/gem5/build"
src_map="-v ${local_gem5_dir}/gem5_restore:/home/gem5_src/gem5/src"
config_map="-v ${local_gem5_dir}/gem5_configs:/home/gem5_src/gem5/configs"
shell_map="-v ${local_gem5_dir}/spec2006_script:/home/gem5_src/gem5/spec2006_script"
ckptinfo="-v ${local_gem5_dir}/ckptinfo:/home/gem5_src/gem5/ckptinfo"
spec2006_map="-v /home/cuihongwei/experiment/spec2006:/root/spec2006"
gem5_data_map="-v /home/cuihongwei/experiment/gem5_dir/gem5_data:/home/gem5_src/gem5/gem5_data"
local_dir_map="-v /home/cuihongwei/experiment/local_dir:/root/local_dir"

map=$build_map" "$src_map" "$config_map" "$shell_map" "$spec2006_map" "$gem5_data_map" "$ckptinfo
docker_run="/home/gem5_src/gem5/spec2006_script/single_run.sh $1 1 x86; exit;"

if [[ $# == 1 ]]; then
    podman run -it --rm $map registry.cn-hangzhou.aliyuncs.com/ronglonely/gem5:3.0 /bin/bash -c "$docker_run"
else
    podman run -it --rm $map $local_dir_map  registry.cn-hangzhou.aliyuncs.com/ronglonely/gem5:3.0 /bin/bash
fi
