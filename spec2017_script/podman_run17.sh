#!/bin/bash
build_map="-v /home/cuihongwei/experiment/gem5_dir/gem5_work/build:/home/gem5_src/gem5/build"
src_map="-v /home/cuihongwei/experiment/gem5_dir/gem5_work/gem5_src:/home/gem5_src/gem5/src"
shell06_map="-v /home/cuihongwei/experiment/gem5_dir/gem5_work/spec2006_script:/home/gem5_src/gem5/spec2006_script"
spec2006_map="-v /home/cuihongwei/experiment/spec2017:/root/spec2006"
shell17_map="-v /home/cuihongwei/experiment/gem5_dir/gem5_work/spec2017_script:/home/gem5_src/gem5/spec2017_script"
spec2017_map="-v /home/cuihongwei/experiment/spec2017:/root/spec2017"

gem5_data_map="-v /home/cuihongwei/experiment/gem5_dir/gem5_data:/home/gem5_src/gem5/gem5_data"
local_dir_map="-v /home/cuihongwei/experiment/local_dir:/root/local_dir"

map=$build_map" "$src_map" "$shell06_map" "$spec2006_map" "$shell17_map" "$spec2017_map" "$gem5_data_map
docker_run="/home/gem5_src/gem5/spec2017_script/simple_run17.sh $1 1 x86; exit;"

if [[ $# == 1 ]]; then
    podman run -it --rm $map registry.cn-hangzhou.aliyuncs.com/ronglonely/gem5:3.0 /bin/bash -c "$docker_run"
else
    podman run -it --rm $map $local_dir_map  registry.cn-hangzhou.aliyuncs.com/ronglonely/gem5:3.0 /bin/bash
fi
