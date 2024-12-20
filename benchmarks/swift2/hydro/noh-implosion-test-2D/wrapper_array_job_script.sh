#!/bin/sh

CLANG_STATUS="on"   

CORES=(1 2 4 8 16 20 24 28 32 36 40 44 48 52 56 60 64)

for cores in "${CORES[@]}"
do
  sbatch -J $CLANG_STATUS"_"$cores array_submit_job_script.sh $cores
done
