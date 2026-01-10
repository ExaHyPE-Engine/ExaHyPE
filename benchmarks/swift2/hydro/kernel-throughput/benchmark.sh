#!/usr/bin/env bash

export PYTHONPATH=../../../../python

ARCH=mi300a
NUMA_PREAMBLE=""

for CELL_SIZE in 64 128 256 512 1024
do
  sed -i 's/\(const int Benchmark_MinParticles = \)[0-9]\+;/\1'"$CELL_SIZE"';/' Constants.h
  sed -i 's/\(const int Benchmark_WorkItemSize = \)[0-9]\+;/\1'"$CELL_SIZE"';/' Constants.h
  rm BenchmarkDriver.o
  make
  echo "Running soatrue cell($CELL_SIZE) at $(date)"
  $NUMA_PREAMBLE ./benchmark > out-$ARCH-soatrue-cell$CELL_SIZE.log
done
