#!/bin/bash

local_gem5_dir=./createCkpt

if [[ ! -e $local_gem5_dir ]]; then
    echo gem5_dir is not correct, $local_gem5_dir
    exit
fi

build_map="-v ${local_gem5_dir}/build:/home/gem5_src/gem5/build"
config_map="-v ${local_gem5_dir}/gem5_configs:/home/gem5_src/gem5/configs"
src_map="-v ${local_gem5_dir}/gem5_create:/home/gem5_src/gem5/src"
test_map="-v ${local_gem5_dir}/testenv:/home/gem5_src/gem5/testenv"

map=$build_map" "$src_map" "$test_map" "$config_map

podman run -it --rm $map  registry.cn-hangzhou.aliyuncs.com/ronglonely/gem5:3.0 /bin/bash