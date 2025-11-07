#!/usr/bin/env bash

export PYTHONPATH=../../../../python

ARCH=gh
NUMA_PREAMBLE=""

python3 ./kernel-throughput.py -d 2 -maxp 1048576 -minp 64 -wis 64 -o benchmark-offload
perl -i -pe 's|observers/\S+\.\S+||g' Makefile

for CELL_SIZE in 64 128 256 512 1024
do
  sed -i 's/\(const int Benchmark_MinParticles = \)[0-9]\+;/\1'"$CELL_SIZE"';/' Constants.h
  sed -i 's/\(const int Benchmark_WorkItemSize = \)[0-9]\+;/\1'"$CELL_SIZE"';/' Constants.h
  rm BenchmarkDriver.o
  make
  echo "Running cell($CELL_SIZE) at $(date)"
  $NUMA_PREAMBLE ./benchmark-offload > out-$ARCH-soatrue-cell$CELL_SIZE.log
done
