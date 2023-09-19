#!/bin/bash
#SBATCH --job-name="run-and-plot2d"
#SBATCH -o run-and-plot.%A.out
#SBATCH -e run-and-plot.%A.err
#SBATCH -p tpg-gpu-small
#SBATCH -w gpu10
#SBATCH -t 03:00:00
#SBATCH -N 1
#SBATCH --gres gpu:1


# --gpus selects the device id you see on sycl-ls
for i in 4 6 8; do
	./kernel-benchmarks-fv-rusanov-2d-patch-size-$i --threads 1 --gpus 2 > sycl-2d-ps$i.txt
done


python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv batched -t 50 -ps 4 6 8

python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv task-graph -t 50 -ps 4 6 8

python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv patch-wise -t 50 -ps 4 6 8

python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv batched -t 50 -ds AoSoA -ps 4 6 8

python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv task-graph -t 50 -ds AoSoA -ps 4 6 8

python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv patch-wise -t 50 -ds AoSoA -ps 4 6 8

python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv batched -t 50 -ds SoA -ps 4 6 8

python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv task-graph -t 50 -ds SoA -ps 4 6 8

python3 create-exahype-sycl-plot.py -f sycl-2d-ps4.txt sycl-2d-ps6.txt sycl-2d-ps8.txt -kv patch-wise -t 50 -ds SoA -ps 4 6 8

