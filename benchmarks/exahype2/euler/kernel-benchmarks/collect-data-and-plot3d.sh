#!/bin/bash
#SBATCH --job-name="run-and-plot3d"
#SBATCH -o run-and-plot.%A.out
#SBATCH -e run-and-plot.%A.err
#SBATCH -p tpg-gpu-small
#SBATCH -w gpu10
#SBATCH -t 05:00:00
#SBATCH -N 1
#SBATCH --gres gpu:1


# --gpus selects the device id you see on sycl-ls
for i in 4 6 8; do
	./kernel-benchmarks-fv-rusanov-3d-patch-size-$i --threads 1 --gpus 2 > sycl-3d-ps$i.txt
done


python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv batched -t 50 -ps 4 6 8 -d 3

python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv task-graph -t 50 -ps 4 6 8 -d 3

python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv patch-wise -t 50 -ps 4 6 8 -d 3

python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv batched -t 50 -ps 4 6 8 -d 3 -ds AoSoA

python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv task-graph -t 50 -ps 4 6 8 -d 3 -ds AoSoA

python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv patch-wise -t 50 -ps 4 6 8 -d 3 -ds AoSoA

python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv batched -t 50 -ps 4 6 8 -d 3 -ds SoA

python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv task-graph -t 50 -ps 4 6 8 -d 3 -ds SoA

python3 create-exahype-sycl-plot.py -f sycl-3d-ps4.txt sycl-3d-ps6.txt sycl-3d-ps8.txt -kv patch-wise -t 50 -ps 4 6 8 -d 3 -ds SoA




