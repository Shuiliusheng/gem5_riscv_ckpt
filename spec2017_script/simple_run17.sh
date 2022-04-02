
#!/bin/bash

#simpoint1 is copying the nessary running files to another directory

#判断输入是否合法
ARGC=$# 
if [[ "$ARGC" < 3 ]]
then
	echo "./xxx.sh 400.perlbench(400, perlbench) number(test=0, ref=1,2...) (arm alpha amd64)"
    exit
fi

#arm riscv mips alpha amd64
if [ $3 = "arm" -o $3 = "ARM" ]; then
	ISA="arm"
	gem5_arch=ARM/gem5.opt
elif [ $3 = "alpha" -o $3 = "ALPHA" ]; then
	ISA="alpha"
	gem5_arch=ALPHA/gem5.opt
else
	ISA="amd64"
	gem5_arch=X86/gem5.opt	
fi


# ************************** spec 2017 parameters ******************************
spec_dir=/root/spec2017
gem5_dir=/home/gem5_src/gem5
gem5_config=configs/example/se.py
gem5_output_dir=$gem5_dir/gem5_data

debug_flag="" #--debug-flags=ShowRunning

#--maxinsts=100000000 --fast-forward=10000000
run_args="--maxinsts=5000000000"
#o3 args: "--cpu-type=DerivO3CPU  --mem-size=8GB --l1d_size=64kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=4 --l2cache --l2_size=2MB --l2_assoc=16 "
#atomic args: "--cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB"
gem5_args="--cpu-type=NonCachingSimpleCPU --mem-type=SimpleMemory --mem-size=8GB"
gem5_args=$gem5_args" "$run_args
			

#**************************** benchmark name ************************************
if [ $1 = "500" -o $1 =  "perlbench_r" -o $1 = "500.perlbench_r" ]; then
	data_type="refrate"
	data_dir="500.perlbench_r"
	target="500.perlbench_r"
elif [ $1 = "502" -o $1 =  "gcc_r" -o $1 = "502.gcc_r" ]; then
	data_type="refrate"
	data_dir="502.gcc_r"
	target="502.gcc_r"
elif [ $1 = "503" -o $1 =  "bwaves_r" -o $1 = "503.bwaves_r" ]; then
	data_type="refrate"
	data_dir="503.bwaves_r"
	target="503.bwaves_r"
elif [ $1 = "505" -o $1 =  "mcf_r" -o $1 = "505.mcf_r" ]; then
	data_type="refrate"
	data_dir="505.mcf_r"
	target="505.mcf_r"
elif [ $1 = "507" -o $1 =  "cactuBSSN_r" -o $1 = "507.cactuBSSN_r" ]; then
	data_type="refrate"
	data_dir="507.cactuBSSN_r"
	target="507.cactuBSSN_r"
elif [ $1 = "508" -o $1 =  "namd_r" -o $1 = "508.namd_r" ]; then
	data_type="refrate"
	data_dir="508.namd_r"
	target="508.namd_r"
elif [ $1 = "510" -o $1 =  "parest_r" -o $1 = "510.parest_r" ]; then
	data_type="refrate"
	data_dir="510.parest_r"
	target="510.parest_r"
elif [ $1 = "511" -o $1 =  "povray_r" -o $1 = "511.povray_r" ]; then
	data_type="refrate"
	data_dir="511.povray_r"
	target="511.povray_r"
elif [ $1 = "519" -o $1 =  "lbm_r" -o $1 = "519.lbm_r" ]; then
	data_type="refrate"
	data_dir="519.lbm_r"
	target="519.lbm_r"
elif [ $1 = "520" -o $1 =  "omnetpp_r" -o $1 = "520.omnetpp_r" ]; then
	data_type="refrate"
	data_dir="520.omnetpp_r"
	target="520.omnetpp_r"
elif [ $1 = "521" -o $1 =  "wrf_r" -o $1 = "521.wrf_r" ]; then
	data_type="refrate"
	data_dir="521.wrf_r"
	target="521.wrf_r"
elif [ $1 = "523" -o $1 =  "xalancbmk_r" -o $1 = "523.xalancbmk_r" ]; then
	data_type="refrate"
	data_dir="523.xalancbmk_r"
	target="523.xalancbmk_r"
elif [ $1 = "525" -o $1 =  "x264_r" -o $1 = "525.x264_r" ]; then
	data_type="refrate"
	data_dir="525.x264_r"
	target="525.x264_r"
elif [ $1 = "526" -o $1 =  "blender_r" -o $1 = "526.blender_r" ]; then
	data_type="refrate"
	data_dir="526.blender_r"
	target="526.blender_r"
elif [ $1 = "527" -o $1 =  "cam4_r" -o $1 = "527.cam4_r" ]; then
	data_type="refrate"
	data_dir="527.cam4_r"
	target="527.cam4_r"
elif [ $1 = "531" -o $1 =  "deepsjeng_r" -o $1 = "531.deepsjeng_r" ]; then
	data_type="refrate"
	data_dir="531.deepsjeng_r"
	target="531.deepsjeng_r"
elif [ $1 = "538" -o $1 =  "imagick_r" -o $1 = "538.imagick_r" ]; then
	data_type="refrate"
	data_dir="538.imagick_r"
	target="538.imagick_r"
elif [ $1 = "541" -o $1 =  "leela_r" -o $1 = "541.leela_r" ]; then
	data_type="refrate"
	data_dir="541.leela_r"
	target="541.leela_r"
elif [ $1 = "544" -o $1 =  "nab_r" -o $1 = "544.nab_r" ]; then
	data_type="refrate"
	data_dir="544.nab_r"
	target="544.nab_r"
elif [ $1 = "548" -o $1 =  "exchange2_r" -o $1 = "548.exchange2_r" ]; then
	data_type="refrate"
	data_dir="548.exchange2_r"
	target="548.exchange2_r"
elif [ $1 = "549" -o $1 =  "fotonik3d_r" -o $1 = "549.fotonik3d_r" ]; then
	data_type="refrate"
	data_dir="549.fotonik3d_r"
	target="549.fotonik3d_r"
elif [ $1 = "554" -o $1 =  "roms_r" -o $1 = "554.roms_r" ]; then
	data_type="refrate"
	data_dir="554.roms_r"
	target="554.roms_r"
elif [ $1 = "557" -o $1 =  "xz_r" -o $1 = "557.xz_r" ]; then
	data_type="refrate"
	data_dir="557.xz_r"
	target="557.xz_r"
elif [ $1 = "600" -o $1 =  "perlbench_s" -o $1 = "600.perlbench_s" ]; then
	data_type="refrate"
	data_dir="500.perlbench_r"
	target="600.perlbench_s"
elif [ $1 = "602" -o $1 =  "gcc_s" -o $1 = "602.gcc_s" ]; then
	data_type="refspeed"
	data_dir="502.gcc_r"
	target="602.gcc_s"
elif [ $1 = "603" -o $1 =  "bwaves_s" -o $1 = "603.bwaves_s" ]; then
	data_type="refspeed"
	data_dir="503.bwaves_r"
	target="603.bwaves_s"
elif [ $1 = "605" -o $1 =  "mcf_s" -o $1 = "605.mcf_s" ]; then
	data_type="refspeed"
	data_dir="505.mcf_r"
	target="605.mcf_s"
elif [ $1 = "607" -o $1 =  "cactuBSSN_s" -o $1 = "607.cactuBSSN_s" ]; then
	data_type="refspeed"
	data_dir="507.cactuBSSN_r"
	target="607.cactuBSSN_s"
elif [ $1 = "619" -o $1 =  "lbm_s" -o $1 = "619.lbm_s" ]; then
	data_type="refspeed"
	data_dir="619.lbm_s"
	target="619.lbm_s"
elif [ $1 = "620" -o $1 =  "omnetpp_s" -o $1 = "620.omnetpp_s" ]; then
	data_type="refrate"
	data_dir="520.omnetpp_r"
	target="620.omnetpp_s"
elif [ $1 = "621" -o $1 =  "wrf_s" -o $1 = "621.wrf_s" ]; then
	data_type="refspeed"
	data_dir="521.wrf_r"
	target="621.wrf_s"
elif [ $1 = "623" -o $1 =  "xalancbmk_s" -o $1 = "623.xalancbmk_s" ]; then
	data_type="refrate"
	data_dir="523.xalancbmk_r"
	target="623.xalancbmk_s"
elif [ $1 = "625" -o $1 =  "x264_s" -o $1 = "625.x264_s" ]; then
	data_type="refrate"
	data_dir="525.x264_r"
	target="625.x264_s"
elif [ $1 = "627" -o $1 =  "cam4_s" -o $1 = "627.cam4_s" ]; then
	data_type="refspeed"
	data_dir="527.cam4_r"
	target="627.cam4_s"
elif [ $1 = "628" -o $1 =  "pop2_s" -o $1 = "628.pop2_s" ]; then
	data_type="refspeed"
	data_dir="628.pop2_s"
	target="628.pop2_s"
elif [ $1 = "631" -o $1 =  "deepsjeng_s" -o $1 = "631.deepsjeng_s" ]; then
	data_type="refspeed"
	data_dir="631.deepsjeng_s"
	target="631.deepsjeng_s"
elif [ $1 = "638" -o $1 =  "imagick_s" -o $1 = "638.imagick_s" ]; then
	data_type="refspeed"
	data_dir="538.imagick_r"
	target="638.imagick_s"
elif [ $1 = "641" -o $1 =  "leela_s" -o $1 = "641.leela_s" ]; then
	data_type="refrate"
	data_dir="541.leela_r"
	target="641.leela_s"
elif [ $1 = "644" -o $1 =  "nab_s" -o $1 = "644.nab_s" ]; then
	data_type="refspeed"
	data_dir="544.nab_r"
	target="644.nab_s"
elif [ $1 = "648" -o $1 =  "exchange2_s" -o $1 = "648.exchange2_s" ]; then
	data_type="refrate"
	data_dir="548.exchange2_r"
	target="648.exchange2_s"
elif [ $1 = "649" -o $1 =  "fotonik3d_s" -o $1 = "649.fotonik3d_s" ]; then
	data_type="refspeed"
	data_dir="549.fotonik3d_r"
	target="649.fotonik3d_s"
elif [ $1 = "654" -o $1 =  "roms_s" -o $1 = "654.roms_s" ]; then
	data_type="refspeed"
	data_dir="554.roms_r"
	target="654.roms_s"
elif [ $1 = "657" -o $1 =  "xz_s" -o $1 = "657.xz_s" ]; then
	data_type="refspeed"
	data_dir="557.xz_r"
	target="657.xz_s"
else
	echo "input wrong!"
	exit
fi


#******************************** necessary directory *****************

curTime=$(date "+%y%m%d-%H%M%S")
#Gem5的输出文件目录，包括stats的输出文件等
gem5_output_dir=$gem5_output_dir/$target/$ISA-$target-"input"$2-$curTime

#运行目录，具体到可以支持benchmark可以运行的目录
run_dir=$gem5_output_dir/run
mkdir -p $run_dir

#benchmark的输出目录
benchmark_output=$run_dir
#记录执行参数的脚本
script_out=$gem5_output_dir/runscript.log

#*********************** spec2006 cmd *********************
# --cmd

exe_suffix="_base.$ISA"
# without running directory
cmd_perlbench_r="perlbench_r"$exe_suffix
cmd_gcc_r="gcc_r"$exe_suffix
cmd_bwaves_r="bwaves_r"$exe_suffix
cmd_mcf_r="mcf_r"$exe_suffix
cmd_cactuBSSN_r="cactuBSSN_r"$exe_suffix
cmd_namd_r="namd_r"$exe_suffix
cmd_parest_r="parest_r"$exe_suffix
cmd_povray_r="povray_r"$exe_suffix
cmd_lbm_r="lbm_r"$exe_suffix
cmd_omnetpp_r="omnetpp_r"$exe_suffix
cmd_wrf_r="wrf_r"$exe_suffix
cmd_xalancbmk_r="xalancbmk_r"$exe_suffix
cmd_x264_r="x264_r"$exe_suffix
cmd_blender_r="blender_r"$exe_suffix
cmd_cam4_r="cam4_r"$exe_suffix
cmd_deepsjeng_r="deepsjeng_r"$exe_suffix
cmd_imagick_r="imagick_r"$exe_suffix
cmd_leela_r="leela_r"$exe_suffix
cmd_nab_r="nab_r"$exe_suffix
cmd_exchange2_r="exchange2_r"$exe_suffix
cmd_fotonik3d_r="fotonik3d_r"$exe_suffix
cmd_roms_r="roms_r"$exe_suffix
cmd_xz_r="xz_r"$exe_suffix

cmd_perlbench_s="perlbench_s"$exe_suffix
cmd_gcc_s="gcc_s"$exe_suffix
cmd_bwaves_s="bwaves_s"$exe_suffix
cmd_mcf_s="mcf_s"$exe_suffix
cmd_cactuBSSN_s="cactuBSSN_s"$exe_suffix
cmd_lbm_s="lbm_s"$exe_suffix
cmd_omnetpp_s="omnetpp_s"$exe_suffix
cmd_wrf_s="wrf_s"$exe_suffix
cmd_xalancbmk_s="xalancbmk_s"$exe_suffix
cmd_x264_s="x264_s"$exe_suffix
cmd_cam4_s="cam4_s"$exe_suffix
cmd_pop2_s="pop2_s"$exe_suffix
cmd_deepsjeng_s="deepsjeng_s"$exe_suffix
cmd_imagick_s="imagick_s"$exe_suffix
cmd_leela_s="leela_s"$exe_suffix
cmd_nab_s="nab_s"$exe_suffix
cmd_exchange2_s="exchange2_s"$exe_suffix
cmd_fotonik3d_s="fotonik3d_s"$exe_suffix
cmd_roms_s="roms_s"$exe_suffix
cmd_xz_s="xz_s"$exe_suffix



#************************ benchmark parameters ****************
# --options
#############################
args_perlbench_r[0]="-I. -I./lib makerand.pl"
args_perlbench_r[1]="-I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1"
args_perlbench_r[2]="-I./lib diffmail.pl 4 800 10 17 19 300"
args_perlbench_r[3]="-I./lib splitmail.pl 6400 12 26 16 100 0"

name_perlbench_r[3]="splitmail.6400.12.26.16.100.0"
name_perlbench_r[2]="diffmail.4.800.10.17.19.300"
name_perlbench_r[1]="checkspam.2500.5.25.11.150.1.1.1.1"
name_perlbench_r[0]="makerand"

num_perlbench_r=4


#############################
args_perlbench_s[0]="-I. -I./lib makerand.pl"
args_perlbench_s[1]="-I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1"
args_perlbench_s[2]="-I./lib diffmail.pl 4 800 10 17 19 300"
args_perlbench_s[3]="-I./lib splitmail.pl 6400 12 26 16 100 0"

name_perlbench_s[3]="splitmail.6400.12.26.16.100.0"
name_perlbench_s[2]="diffmail.4.800.10.17.19.300"
name_perlbench_s[1]="checkspam.2500.5.25.11.150.1.1.1.1"
name_perlbench_s[0]="makerand"
num_perlbench_s=4

#############################
args_gcc_r[0]="t1.c -O3 -finline-limit=50000 -o t1.opts-O3_-finline-limit_50000.s"
args_gcc_r[1]="gcc-pp.c -O3 -finline-limit=0 -fif-conversion -fif-conversion2 -o gcc-pp.opts-O3_-finline-limit_0_-fif-conversion_-fif-conversion2.s"
args_gcc_r[2]="gcc-pp.c -O2 -finline-limit=36000 -fpic -o gcc-pp.opts-O2_-finline-limit_36000_-fpic.s"
args_gcc_r[3]="gcc-smaller.c -O3 -fipa-pta -o gcc-smaller.opts-O3_-fipa-pta.s"
args_gcc_r[4]="ref32.c -O5 -o ref32.opts-O5.s"
args_gcc_r[5]="ref32.c -O3 -fselective-scheduling -fselective-scheduling2 -o ref32.opts-O3_-fselective-scheduling_-fselective-scheduling2.s"

name_gcc_r[0]="t1.opts-O3_-finline-limit_50000"
name_gcc_r[1]="gcc-pp.opts-O3_-finline-limit_0_-fif-conversion_-fif-conversion2"
name_gcc_r[2]="gcc-pp.opts-O2_-finline-limit_36000_-fpic"
name_gcc_r[3]="gcc-smaller.opts-O3_-fipa-pta"
name_gcc_r[4]="ref32.opts-O5"
name_gcc_r[5]="ref32.opts-O3_-fselective-scheduling_-fselective-scheduling2"
num_gcc_r=6


#############################
args_gcc_s[0]="t1.c -O3 -finline-limit=50000 -o t1.opts-O3_-finline-limit_50000.s"
args_gcc_s[1]="gcc-pp.c -O5 -fipa-pta -o gcc-pp.opts-O5_-fipa-pta.s"
args_gcc_s[2]="gcc-pp.c -O5 -finline-limit=1000 -fselective-scheduling -fselective-scheduling2 -o gcc-pp.opts-O5_-finline-limit_1000_-fselective-scheduling_-fselective-scheduling2.s"
args_gcc_s[3]="gcc-pp.c -O5 -finline-limit=24000 -fgcse -fgcse-las -fgcse-lm -fgcse-sm -o gcc-pp.opts-O5_-finline-limit_24000_-fgcse_-fgcse-las_-fgcse-lm_-fgcse-sm.s"

name_gcc_s[0]="t1.opts-O3_-finline-limit_50000"
name_gcc_s[1]="gcc-pp.opts-O5_-fipa-pta"
name_gcc_s[2]="gcc-pp.opts-O5_-finline-limit_1000_-fselective-scheduling_-fselective-scheduling2"
name_gcc_s[3]="gcc-pp.opts-O5_-finline-limit_24000_-fgcse_-fgcse-las_-fgcse-lm_-fgcse-sm"
num_gcc_s=4

################################
args_bwaves_r[0]="bwaves_1"
args_bwaves_r[1]="bwaves_1"
args_bwaves_r[2]="bwaves_2"
args_bwaves_r[3]="bwaves_3"
args_bwaves_r[4]="bwaves_4"

input_bwaves_r[0]="bwaves_1.in"
input_bwaves_r[1]="bwaves_1.in"
input_bwaves_r[2]="bwaves_2.in"
input_bwaves_r[3]="bwaves_3.in"
input_bwaves_r[4]="bwaves_4.in"

name_bwaves_r[0]="bwaves_1"
name_bwaves_r[1]="bwaves_1"
name_bwaves_r[2]="bwaves_2"
name_bwaves_r[3]="bwaves_3"
name_bwaves_r[4]="bwaves_4"
num_bwaves_r=5

################################
args_bwaves_s[0]="bwaves_1"
args_bwaves_s[1]="bwaves_1"
args_bwaves_s[2]="bwaves_2"

input_bwaves_s[0]="bwaves_1.in"
input_bwaves_s[1]="bwaves_1.in"
input_bwaves_s[2]="bwaves_2.in"

name_bwaves_s[0]="bwaves_1"
name_bwaves_s[1]="bwaves_1"
name_bwaves_s[2]="bwaves_2"
num_bwaves_s=3
#########################
args_mcf_r[0]="inp.in"
args_mcf_r[1]="inp.in"
name_mcf_r[0]="inp"
name_mcf_r[1]="inp"
num_mcf_r=2

#########################
args_mcf_s[0]="inp.in"
args_mcf_s[1]="inp.in"
name_mcf_s[0]="inp"
name_mcf_s[1]="inp"
num_mcf_s=2

#########################
args_cactuBSSN_r[0]="spec_test.par"
args_cactuBSSN_r[1]="spec_ref.par"

name_cactuBSSN_r[1]="spec_ref"
name_cactuBSSN_r[0]="spec_test"
num_cactuBSSN_r=2

#########################
args_cactuBSSN_s[0]="spec_test.par"
args_cactuBSSN_s[1]="spec_ref.par"

name_cactuBSSN_s[1]="spec_ref"
name_cactuBSSN_s[0]="spec_test"
num_cactuBSSN_s=2

#########################
args_namd_r[0]="--input apoa1.input --iterations 1 --output apoa1.test.output"
args_namd_r[1]="--input apoa1.input --output apoa1.ref.output --iterations 65"
name_namd_r[1]="namd"
name_namd_r[0]="namd"
num_namd_r=2

#########################
args_parest_r[0]="test.prm"
args_parest_r[1]="ref.prm"
name_parest_r[1]="ref"
name_parest_r[0]="test"
num_parest_r=2

#########################
args_povray_r[0]="SPEC-benchmark-test.ini"
args_povray_r[1]="SPEC-benchmark-ref.ini"
name_povray_r[1]="SPEC-benchmark-ref"
name_povray_r[0]="SPEC-benchmark-test"
num_povray_r=2

#########################
args_lbm_r[0]="20 reference.dat 0 1 100_100_130_cf_a.of"
args_lbm_r[1]="3000 reference.dat 0 0 100_100_130_ldc.of"
name_lbm_r[1]="lbm_r"
name_lbm_r[0]="lbm_r"
num_lbm_r=2

#########################
args_lbm_s[0]="20 reference.dat 0 1 200_200_260_ldc.of"
args_lbm_s[1]="2000 reference.dat 0 0 200_200_260_ldc.of"
name_lbm_s[1]="lbm_s"
name_lbm_s[0]="lbm_s"
num_lbm_s=2

#########################
args_omnetpp_r[0]="-c General -r 0"
args_omnetpp_r[1]="-c General -r 0"
name_omnetpp_r[0]="omnetpp.General-0"
name_omnetpp_r[1]="omnetpp.General-0"
num_omnetpp_r=2

#########################
args_omnetpp_s[0]="-c General -r 0"
args_omnetpp_s[1]="-c General -r 0"
name_omnetpp_s[0]="omnetpp.General-0"
name_omnetpp_s[1]="omnetpp.General-0"
num_omnetpp_s=2


#########################
args_wrf_r[0]=""
args_wrf_r[1]=""
name_wrf_r[1]="wrf_r"
name_wrf_r[0]="wrf_r"
num_wrf_r=2

#########################
args_wrf_s[0]=""
args_wrf_s[1]=""
name_wrf_s[1]="wrf_r"
name_wrf_s[0]="wrf_r"
num_wrf_s=2

#########################
args_xalancbmk_r[0]="-v test.xml xalanc.xsl"
args_xalancbmk_r[1]="-v t5.xml xalanc.xsl"
name_xalancbmk_r[1]="ref"
name_xalancbmk_r[0]="test"
num_xalancbmk_r=2

#########################
args_xalancbmk_s[0]="-v test.xml xalanc.xsl"
args_xalancbmk_s[1]="-v t5.xml xalanc.xsl"
name_xalancbmk_s[1]="ref"
name_xalancbmk_s[0]="test"
num_xalancbmk_s=2

#########################
args_x264_r[0]="--dumpyuv 50 --frames 156 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"
args_x264_r[1]="--pass 1 --stats x264_stats.log --bitrate 1000 --frames 1000 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"
args_x264_r[2]="--pass 2 --stats x264_stats.log --bitrate 1000 --dumpyuv 200 --frames 1000 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"
args_x264_r[3]="--seek 500 --dumpyuv 200 --frames 1250 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"

name_x264_r[0]="run_000-156_x264_r_x264"
name_x264_r[1]="run_000-1000_x264_r_x264_pass1"
name_x264_r[2]="run_000-1000_x264_r_x264_pass2"
name_x264_r[3]="run_0500-1250_x264_r_x264"
num_x264_r=4

#########################
args_x264_s[0]="--dumpyuv 50 --frames 156 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"
args_x264_s[1]="--pass 1 --stats x264_stats.log --bitrate 1000 --frames 1000 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"
args_x264_s[2]="--pass 2 --stats x264_stats.log --bitrate 1000 --dumpyuv 200 --frames 1000 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"
args_x264_s[3]="--seek 500 --dumpyuv 200 --frames 1250 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"

name_x264_s[0]="run_000-156_x264_s_x264"
name_x264_s[1]="run_000-1000_x264_s_x264_pass1"
name_x264_s[2]="run_000-1000_x264_s_x264_pass2"
name_x264_s[3]="run_0500-1250_x264_s_x264"
num_x264_s=4

#########################
args_blender_r[0]="cube.blend --render-output cube_ --threads 1 -b -F RAWTGA -s 1 -e 1 -a"
args_blender_r[1]="sh3_no_char.blend --render-output sh3_no_char_ --threads 1 -b -F RAWTGA -s 849 -e 849 -a"
name_blender_r[0]="cube.1.spec"
name_blender_r[1]="sh3_no_char.849.spec"
num_blender_r=2

#########################
args_cam4_r[0]="-v test.xml xalanc.xsl"
args_cam4_r[1]="-v t5.xml xalanc.xsl"
name_cam4_r[0]="cam4_r"
name_cam4_r[1]="cam4_r"
num_cam4_r=2

#########################
args_cam4_s[0]="-v test.xml xalanc.xsl"
args_cam4_s[1]="-v t5.xml xalanc.xsl"
name_cam4_s[0]="cam4_s"
name_cam4_s[1]="cam4_s"
num_cam4_s=2

#########################
args_deepsjeng_r[0]="test.txt"
args_deepsjeng_r[1]="ref.txt"
name_deepsjeng_r[0]="test"
name_deepsjeng_r[1]="ref"
num_deepsjeng_r=2

#########################
args_deepsjeng_s[0]="test.txt"
args_deepsjeng_s[1]="ref.txt"
name_deepsjeng_s[0]="test"
name_deepsjeng_s[1]="ref"
num_deepsjeng_s=2 

#########################
args_imagick_r[0]="-limit disk 0 test_input.tga -shear 25 -resize 640x480 -negate -alpha Off test_output.tga"
args_imagick_r[1]="-limit disk 0 refrate_input.tga -edge 41 -resample 181% -emboss 31 -colorspace YUV -mean-shift 19x19+15% -resize 30% refrate_output.tga"
name_imagick_r[0]="test_convert"
name_imagick_r[1]="refrate_convert"
num_imagick_r=2


#########################
args_imagick_s[0]="-limit disk 0 test_input.tga -shear 25 -resize 640x480 -negate -alpha Off test_output.tga"
args_imagick_s[1]="-limit disk 0 refspeed_input.tga -resize 817% -rotate -2.76 -shave 540x375 -alpha remove -auto-level -contrast-stretch 1x1% -colorspace Lab -channel R -equalize +channel -colorspace sRGB -define histogram:unique-colors=false -adaptive-blur 0x5 -despeckle -auto-gamma -adaptive-sharpen 55 -enhance -brightness-contrast 10x10 -resize 30% refspeed_output.tga"
name_imagick_s[0]="test_convert"
name_imagick_s[1]="refspeed_convert"
num_imagick_s=2

#########################
args_leela_r[0]="test.sgf"
args_leela_r[1]="ref.sgf"
name_leela_r[0]="test"
name_leela_r[1]="ref"
num_leela_r=2

#########################
args_leela_s[0]="test.sgf"
args_leela_s[1]="ref.sgf"
name_leela_s[0]="test"
name_leela_s[1]="ref"
num_leela_s=2 

#########################
args_nab_r[0]="hkrdenq 1930344093 1000"
args_nab_r[1]="1am0 1122214447 122"
name_nab_r[0]="hkrdenq"
name_nab_r[1]="1am0"
num_nab_r=2

#########################
args_nab_s[0]="hkrdenq 1930344093 1000"
args_nab_s[1]="3j1n 20140317 220"
name_nab_s[0]="hkrdenq"
name_nab_s[1]="3j1n"
num_nab_s=2

#########################
args_exchange2_r[0]="0"
args_exchange2_r[1]="6"
name_exchange2_r[0]="exchange2"
name_exchange2_r[1]="exchange2"
num_exchange2_r=2

#########################
args_exchange2_s[0]="0"
args_exchange2_s[1]="6"
name_exchange2_s[0]="exchange2"
name_exchange2_s[1]="exchange2"
num_exchange2_s=2

#########################
args_fotonik3d_r[0]=""
args_fotonik3d_r[1]=""
name_fotonik3d_r[0]="fotonik3d_r"
name_fotonik3d_r[1]="fotonik3d_r"
num_fotonik3d_r=2

#########################
args_fotonik3d_s[0]=""
args_fotonik3d_s[1]=""
name_fotonik3d_s[0]="fotonik3d_s"
name_fotonik3d_s[1]="fotonik3d_s"
num_fotonik3d_s=2

#########################
args_roms_r[0]=""
args_roms_r[1]=""

input_roms_r[0]="ocean_benchmark0.in.x"
input_roms_r[1]="ocean_benchmark2.in.x"

name_roms_r[0]="ocean_benchmark0"
name_roms_r[1]="ocean_benchmark2"
num_roms_r=2

#########################
args_roms_s[0]=""
args_roms_s[1]=""

input_roms_s[0]="ocean_benchmark0.in"
input_roms_s[1]="ocean_benchmark3.in"

name_roms_s[0]="ocean_benchmark0"
name_roms_s[1]="ocean_benchmark3"
num_roms_s=2

#########################
args_xz_r[0]="cpu2006docs.tar.xz 4 055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae 1548636 1555348 0"
args_xz_r[1]="cld.tar.xz 160 19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474 59796407 61004416 6"
args_xz_r[2]="cpu2006docs.tar.xz 250 055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae 23047774 23513385 6e"
args_xz_r[3]="input.combined.xz 250 a841f68f38572a49d86226b7ff5baeb31bd19dc637a922a972b2e6d1257a890f6a544ecab967c313e370478c74f760eb229d4eef8a8d2836d233d3e9dd1430bf 40401484 41217675 7"

name_xz_r[0]="cpu2006docs.tar-4-0"
name_xz_r[1]="cld.tar-160-6"
name_xz_r[2]="cpu2006docs.tar-250-6e"
name_xz_r[3]="input.combined-250-7"
num_xz_r=4

#########################
args_xz_s[0]="cpu2006docs.tar.xz 4 055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae 1548636 1555348 0"
args_xz_s[2]="cpu2006docs.tar.xz 6643 055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae 1036078272 1111795472 4"
args_xz_s[1]="cld.tar.xz 1400 19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474 536995164 539938872 8 > cld.tar-1400-8.out 2"

name_xz_s[0]="cpu2006docs.tar-4-0"
name_xz_s[2]="cpu2006docs.tar-6643-4"
name_xz_s[1]="cld.tar-1400-8"
num_xz_s=3

#########################
args_pop2_s[0]=""
args_pop2_s[1]=""
name_pop2_s[0]="pop2_s"
name_pop2_s[1]="pop2_s"
num_pop2_s=2


#****************************** choose benchmark ******************************
if  [ $target = "500.perlbench_r" ];  then
	cmd=$cmd_perlbench_r
	if [ $2 -lt $num_perlbench_r ]; then
		args=${args_perlbench_r[$2]}
		input=""
		output=${name_perlbench_r[$2]}".out"
		errout=${name_perlbench_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "502.gcc_r" ];  then
	cmd=$cmd_gcc_r
	if [ $2 -lt $num_gcc_r ]; then
		args=${args_gcc_r[$2]}
		input=""
		output=${name_gcc_r[$2]}".out"
		errout=${name_gcc_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "503.bwaves_r" ];  then
	cmd=$cmd_bwaves_r
	if [ $2 -lt $num_bwaves_r ]; then
		args=${args_bwaves_r[$2]}
		input=${input_bwaves_r[$2]}
		output=${name_bwaves_r[$2]}".out"
		errout=${name_bwaves_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "505.mcf_r" ];  then
	cmd=$cmd_mcf_r
	if [ $2 -lt $num_mcf_r ]; then
		args=${args_mcf_r[$2]}
		input=""
		output=${name_mcf_r[$2]}".out"
		errout=${name_mcf_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "507.cactuBSSN_r" ];  then
	cmd=$cmd_cactuBSSN_r
	if [ $2 -lt $num_cactuBSSN_r ]; then
		args=${args_cactuBSSN_r[$2]}
		input=""
		output=${name_cactuBSSN_r[$2]}".out"
		errout=${name_cactuBSSN_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "508.namd_r" ];  then
	cmd=$cmd_namd_r
	if [ $2 -lt $num_namd_r ]; then
		args=${args_namd_r[$2]}
		input=""
		output=${name_namd_r[$2]}".out"
		errout=${name_namd_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "510.parest_r" ];  then
	cmd=$cmd_parest_r
	if [ $2 -lt $num_parest_r ]; then
		args=${args_parest_r[$2]}
		input=""
		output=${name_parest_r[$2]}".out"
		errout=${name_parest_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "511.povray_r" ];  then
	cmd=$cmd_povray_r
	if [ $2 -lt $num_povray_r ]; then
		args=${args_povray_r[$2]}
		input=""
		output=${name_povray_r[$2]}".out"
		errout=${name_povray_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "519.lbm_r" ];  then
	cmd=$cmd_lbm_r
	if [ $2 -lt $num_lbm_r ]; then
		args=${args_lbm_r[$2]}
		input=""
		output=${name_lbm_r[$2]}".out"
		errout=${name_lbm_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "520.omnetpp_r" ];  then
	cmd=$cmd_omnetpp_r
	if [ $2 -lt $num_omnetpp_r ]; then
		args=${args_omnetpp_r[$2]}
		input=""
		output=${name_omnetpp_r[$2]}".out"
		errout=${name_omnetpp_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "521.wrf_r" ];  then
	cmd=$cmd_wrf_r
	if [ $2 -lt $num_wrf_r ]; then
		args=${args_wrf_r[$2]}
		input=""
		output=${name_wrf_r[$2]}".out"
		errout=${name_wrf_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "523.xalancbmk_r" ];  then
	cmd=$cmd_xalancbmk_r
	if [ $2 -lt $num_xalancbmk_r ]; then
		args=${args_xalancbmk_r[$2]}
		input=""
		output=${name_xalancbmk_r[$2]}".out"
		errout=${name_xalancbmk_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "525.x264_r" ];  then
	cmd=$cmd_x264_r
	if [ $2 -lt $num_x264_r ]; then
		args=${args_x264_r[$2]}
		input=""
		output=${name_x264_r[$2]}".out"
		errout=${name_x264_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "526.blender_r" ];  then
	cmd=$cmd_blender_r
	if [ $2 -lt $num_blender_r ]; then
		args=${args_blender_r[$2]}
		input=""
		output=${name_blender_r[$2]}".out"
		errout=${name_blender_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "527.cam4_r" ];  then
	cmd=$cmd_cam4_r
	if [ $2 -lt $num_cam4_r ]; then
		args=${args_cam4_r[$2]}
		input=""
		output=${name_cam4_r[$2]}".out"
		errout=${name_cam4_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "531.deepsjeng_r" ];  then
	cmd=$cmd_deepsjeng_r
	if [ $2 -lt $num_deepsjeng_r ]; then
		args=${args_deepsjeng_r[$2]}
		input=""
		output=${name_deepsjeng_r[$2]}".out"
		errout=${name_deepsjeng_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "538.imagick_r" ];  then
	cmd=$cmd_imagick_r
	if [ $2 -lt $num_imagick_r ]; then
		args=${args_imagick_r[$2]}
		input=""
		output=${name_imagick_r[$2]}".out"
		errout=${name_imagick_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "541.leela_r" ];  then
	cmd=$cmd_leela_r
	if [ $2 -lt $num_leela_r ]; then
		args=${args_leela_r[$2]}
		input=""
		output=${name_leela_r[$2]}".out"
		errout=${name_leela_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "544.nab_r" ];  then
	cmd=$cmd_nab_r
	if [ $2 -lt $num_nab_r ]; then
		args=${args_nab_r[$2]}
		input=""
		output=${name_nab_r[$2]}".out"
		errout=${name_nab_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "548.exchange2_r" ];  then
	cmd=$cmd_exchange2_r
	if [ $2 -lt $num_exchange2_r ]; then
		args=${args_exchange2_r[$2]}
		input=""
		output=${name_exchange2_r[$2]}".out"
		errout=${name_exchange2_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "549.fotonik3d_r" ];  then
	cmd=$cmd_fotonik3d_r
	if [ $2 -lt $num_fotonik3d_r ]; then
		args=${args_fotonik3d_r[$2]}
		input=""
		output=${name_fotonik3d_r[$2]}".out"
		errout=${name_fotonik3d_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "554.roms_r" ];  then
	cmd=$cmd_roms_r
	if [ $2 -lt $num_roms_r ]; then
		args=${args_roms_r[$2]}
		input=${input_roms_r[$2]}
		output=${name_roms_r[$2]}".out"
		errout=${name_roms_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "557.xz_r" ];  then
	cmd=$cmd_xz_r
	if [ $2 -lt $num_xz_r ]; then
		args=${args_xz_r[$2]}
		input=""
		output=${name_xz_r[$2]}".out"
		errout=${name_xz_r[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "600.perlbench_s" ];  then
	cmd=$cmd_perlbench_s
	if [ $2 -lt $num_perlbench_s ]; then
		args=${args_perlbench_s[$2]}
		input=""
		output=${name_perlbench_s[$2]}".out"
		errout=${name_perlbench_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "602.gcc_s" ];  then
	cmd=$cmd_gcc_s
	if [ $2 -lt $num_gcc_s ]; then
		args=${args_gcc_s[$2]}
		input=""
		output=${name_gcc_s[$2]}".out"
		errout=${name_gcc_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "603.bwaves_s" ];  then
	cmd=$cmd_bwaves_s
	if [ $2 -lt $num_bwaves_s ]; then
		args=${args_bwaves_s[$2]}
		input=${input_bwaves_s[$2]}
		output=${name_bwaves_s[$2]}".out"
		errout=${name_bwaves_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "605.mcf_s" ];  then
	cmd=$cmd_mcf_s
	if [ $2 -lt $num_mcf_s ]; then
		args=${args_mcf_s[$2]}
		input=""
		output=${name_mcf_s[$2]}".out"
		errout=${name_mcf_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "607.cactuBSSN_s" ];  then
	cmd=$cmd_cactuBSSN_s
	if [ $2 -lt $num_cactuBSSN_s ]; then
		args=${args_cactuBSSN_s[$2]}
		input=""
		output=${name_cactuBSSN_s[$2]}".out"
		errout=${name_cactuBSSN_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "619.lbm_s" ];  then
	cmd=$cmd_lbm_s
	if [ $2 -lt $num_lbm_s ]; then
		args=${args_lbm_s[$2]}
		input=""
		output=${name_lbm_s[$2]}".out"
		errout=${name_lbm_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "620.omnetpp_s" ];  then
	cmd=$cmd_omnetpp_s
	if [ $2 -lt $num_omnetpp_s ]; then
		args=${args_omnetpp_s[$2]}
		input=""
		output=${name_omnetpp_s[$2]}".out"
		errout=${name_omnetpp_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "621.wrf_s" ];  then
	cmd=$cmd_wrf_s
	if [ $2 -lt $num_wrf_s ]; then
		args=${args_wrf_s[$2]}
		input=""
		output=${name_wrf_s[$2]}".out"
		errout=${name_wrf_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "623.xalancbmk_s" ];  then
	cmd=$cmd_xalancbmk_s
	if [ $2 -lt $num_xalancbmk_s ]; then
		args=${args_xalancbmk_s[$2]}
		input=""
		output=${name_xalancbmk_s[$2]}".out"
		errout=${name_xalancbmk_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "625.x264_s" ];  then
	cmd=$cmd_x264_s
	if [ $2 -lt $num_x264_s ]; then
		args=${args_x264_s[$2]}
		input=""
		output=${name_x264_s[$2]}".out"
		errout=${name_x264_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "627.cam4_s" ];  then
	cmd=$cmd_cam4_s
	if [ $2 -lt $num_cam4_s ]; then
		args=${args_cam4_s[$2]}
		input=""
		output=${name_cam4_s[$2]}".out"
		errout=${name_cam4_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "628.pop2_s" ];  then
	cmd=$cmd_pop2_s
	if [ $2 -lt $num_pop2_s ]; then
		args=${args_pop2_s[$2]}
		input=""
		output=${name_pop2_s[$2]}".out"
		errout=${name_pop2_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "631.deepsjeng_s" ];  then
	cmd=$cmd_deepsjeng_s
	if [ $2 -lt $num_deepsjeng_s ]; then
		args=${args_deepsjeng_s[$2]}
		input=""
		output=${name_deepsjeng_s[$2]}".out"
		errout=${name_deepsjeng_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "638.imagick_s" ];  then
	cmd=$cmd_imagick_s
	if [ $2 -lt $num_imagick_s ]; then
		args=${args_imagick_s[$2]}
		input=""
		output=${name_imagick_s[$2]}".out"
		errout=${name_imagick_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "641.leela_s" ];  then
	cmd=$cmd_leela_s
	if [ $2 -lt $num_leela_s ]; then
		args=${args_leela_s[$2]}
		input=""
		output=${name_leela_s[$2]}".out"
		errout=${name_leela_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "644.nab_s" ];  then
	cmd=$cmd_nab_s
	if [ $2 -lt $num_nab_s ]; then
		args=${args_nab_s[$2]}
		input=""
		output=${name_nab_s[$2]}".out"
		errout=${name_nab_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "648.exchange2_s" ];  then
	cmd=$cmd_exchange2_s
	if [ $2 -lt $num_exchange2_s ]; then
		args=${args_exchange2_s[$2]}
		input=""
		output=${name_exchange2_s[$2]}".out"
		errout=${name_exchange2_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "649.fotonik3d_s" ];  then
	cmd=$cmd_fotonik3d_s
	if [ $2 -lt $num_fotonik3d_s ]; then
		args=${args_fotonik3d_s[$2]}
		input=""
		output=${name_fotonik3d_s[$2]}".out"
		errout=${name_fotonik3d_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "654.roms_s" ];  then
	cmd=$cmd_roms_s
	if [ $2 -lt $num_roms_s ]; then
		args=${args_roms_s[$2]}
		input=${input_roms_s[$2]}
		output=${name_roms_s[$2]}".out"
		errout=${name_roms_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
elif  [ $target = "657.xz_s" ];  then
	cmd=$cmd_xz_s
	if [ $2 -lt $num_xz_s ]; then
		args=${args_xz_s[$2]}
		input=""
		output=${name_xz_s[$2]}".out"
		errout=${name_xz_s[$2]}".err"
	else
		echo "$ is too big"
		exit
	fi
else
	echo "input wrong!"
	exit
fi

#*******************************start execution************************

echo "" | tee $script_out
echo "******************************directory****************************" | tee -a $script_out
echo "spec_dir: "$spec_dir | tee -a $script_out
echo "run_dir:  "$run_dir | tee -a $script_out
echo "gem5_dir: "$gem5_dir | tee -a $script_out
echo "gem5_output_dir: "$gem5_output_dir | tee -a $script_out
echo "benchmark_output:"$gem5_output_dir | tee -a $script_out
echo "" | tee -a $script_out

echo "******************************benchmark****************************" | tee -a $script_out
echo "target:     "$target | tee -a $script_out
echo "executable: "$cmd | tee -a $script_out
echo "args:       "$args | tee -a $script_out
echo "input:      "$input | tee -a $script_out
echo "output:     "${benchmark_output}/$output | tee -a $script_out
echo "errout:     "${benchmark_output}/$errout | tee -a $script_out
echo "" | tee -a $script_out

echo "******************************gem5****************************" | tee -a $script_out
echo "arch:       "$gem5_dir/build/$gem5_arch | tee -a $script_out
echo "config:     "$gem5_dir/$gem5_config | tee -a $script_out
echo "args:       "$gem5_args | tee -a $script_out
echo "" | tee -a $script_out


echo "******************************simulation****************************" | tee -a $script_out
echo "start time:(year-mon-day hour-minute-second)"$(date "+%y-%m-%d %H-%M-%S")| tee -a $script_out


#进入工作目录
cd $run_dir
mkdir standout
#copy all running need thing
echo "cp $spec_dir/benchspec/CPU/$target/exe/$cmd $run_dir/$cmd"| tee -a $script_out
cp $spec_dir/benchspec/CPU/$target/exe/$cmd $run_dir/$cmd

echo "cp -r $spec_dir/benchspec/CPU/$data_dir/data/all/input/* $run_dir"| tee -a $script_out
cp -r $spec_dir/benchspec/CPU/$data_dir/data/all/input/* $run_dir
#cp -r $spec_dir/benchspec/CPU/$target/run/run_base_ref_amd64.0001/* $run_dir
if [ $2 = "0" ]; then
	echo "cp -r $spec_dir/benchspec/CPU/$data_dir/data/test/input/* $run_dir"| tee -a $script_out
	cp -r $spec_dir/benchspec/CPU/$data_dir/data/test/input/* $run_dir
	cp -r $spec_dir/benchspec/CPU/$data_dir/data/test/output/* $run_dir/standout
	echo "cp -r $spec_dir/benchspec/CPU/$data_dir/data/test/output/* $run_dir/standout"| tee -a $script_out
else
	cp -r $spec_dir/benchspec/CPU/$data_dir/data/$data_type/input/* $run_dir
	echo "cp -r $spec_dir/benchspec/CPU/$data_dir/data/$data_type/input/* $run_dir"| tee -a $script_out
	if [ "${target:0:1}" = "6" ];then
		cp -r $spec_dir/benchspec/CPU/$target/run/run_base_refspeed_mytest-m64.0000/* $run_dir
		echo "cp -r $spec_dir/benchspec/CPU/$target/run/run_base_refspeed_mytest-m64.0000/* $run_dir"| tee -a $script_out
	else
		cp -r $spec_dir/benchspec/CPU/$target/run/run_base_refrate_mytest-m64.0000/* $run_dir
		echo "cp -r $spec_dir/benchspec/CPU/$target/run/run_base_refrate_mytest-m64.0000/* $run_dir"| tee -a $script_out
	fi
	cp -r $spec_dir/benchspec/CPU/$data_dir/data/$data_type/output/* $run_dir/standout
	echo "cp -r $spec_dir/benchspec/CPU/$data_dir/data/$data_type/output/* $run_dir/standout"| tee -a $script_out
fi

#************************************ run gem5 **************************************/
$gem5_dir/build/$gem5_arch \
 	--outdir=$gem5_output_dir  $debug_flag \
 	$gem5_dir/$gem5_config $gem5_args \
 	--cmd=$cmd --options="$args" \
 	--output=${benchmark_output}/$output --input=$input \
 	--errout=${benchmark_output}/$errout | tee -a $script_out
	
echo "end time:(year-mon-day hour-minute-second)"$(date "+%y-%m-%d %H-%M-%S")| tee -a $script_out

rm $run_dir -rf