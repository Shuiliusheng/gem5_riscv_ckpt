#!/bin/bash
ARGC=$#
if [[ "$ARGC" < 1 ]]
then
    echo "./xxx.sh input"
    exit
fi

run_dir=/home/gem5_src/gem5/spec2017_script
script=./podman_run17.sh
file=$1
for bench in `cat $file`
do
	if [ "$bench" = "wait" ];then
		echo is waitting for 1h30m
		sleep 20000
		continue
	fi
	tmux new -s "cmov-"$bench -d
	tmux send-keys -t "cmov-"$bench "cd $run_dir" C-m   
	tmux send-keys -t "cmov-"$bench "$script $bench" C-m
done
