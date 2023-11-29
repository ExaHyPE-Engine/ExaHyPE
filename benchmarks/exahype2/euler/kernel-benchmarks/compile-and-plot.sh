#!/bin/bash
#SBATCH --job-name="exahype-sycl"
#SBATCH -o exahype-sycl.%A.out
#SBATCH -e exahype-sycl.%A.err
#SBATCH -p tpg-gpu-small
#SBATCH -w gpu10 
#SBATCH -t 04:00:00
#SBATCH -N 1


. /opt/intel/oneapi/setvars.sh --include-intel-llvm
export ONEAPI_DEVICE_SELECTOR=level_zero:0

#MODIFY ABOVE AS APPROPRIATE

for i in 2 3; do
    for j in 4 6 8; do
        python kernel-benchmarks-fv-rusanov.py -d $i -ps $j
	    ./kernel-benchmarks-fv-rusanov-${i}d-patch-size-$j &> PVC-d${i}-ps$j.txt
    done
	python create-exahype-sycl-plot.py -f PVC-d${i}-ps4.txt PVC-d${i}-ps6.txt PVC-d${i}-ps8.txt -d $i -ps 4 6 8 -dn PVC -ds AoS
done


#Feel free to change the benchmark configurations


