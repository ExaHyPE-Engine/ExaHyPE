#!/usr/bin/env bash

export PYTHONPATH=../../../../python

ARCH=x64
NUMA_PREAMBLE="numactl --cpunodebind=0 --membind=0"

for CELL_SIZE in 0064 0128 0256 0512 1024
do
  for ENABLE_PREDICATES in false
  do
    for ENABLE_SYMMETRIC_INTERACTION in false
    do
      for ENABLE_SOA_CONVERSION in true false
      do
        python3 ./kernel-throughput.py -d 2 -maxp 1048576 -minp $CELL_SIZE -wis $CELL_SIZE --enable-predicates=$ENABLE_PREDICATES --enable-symmetric-interaction=$ENABLE_SYMMETRIC_INTERACTION --enable-soa-conversion=$ENABLE_SOA_CONVERSION --enable-soa-offload=$ENABLE_SOA_CONVERSION -o benchmark-offload
        perl -i -pe 's|observers/\S+\.\S+||g' Makefile
        make -j8
        echo "Running soa($ENABLE_SOA_CONVERSION) offload($ENABLE_SOA_CONVERSION) cell($CELL_SIZE) predicates($ENABLE_PREDICATES) symmetry($ENABLE_SYMMETRIC_INTERACTION) at $(date)"
        $NUMA_PREAMBLE ./benchmark-offload > out-$ARCH-soa$ENABLE_SOA_CONVERSION-cell$CELL_SIZE-pred$ENABLE_PREDICATES-sym$ENABLE_SYMMETRIC_INTERACTION-gpu$ENABLE_SOA_CONVERSION.log
      done
    done
  done
done
