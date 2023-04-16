#!/bin/bash

# Runs the exahype2 postprocessing script to parse the log file

# Postprocessing script to invoke
SCRIPT=../../../../python/exahype2/postprocessing/plot-scaling.py

# Simulation parameters
CLANG_STATUS=$1
N_PART=$2

# Plot setup
MAX_CORES_PER_RANK=-1   # -1: single rank (node)
LABELS=N_p={$N_PART}^2

# Input data
#DIR_IN=output_np_"$N_PART"_et_0.01_"$CLANG_STATUS"/
DIR_IN=$3/
DIR_OUT=$DIR_IN
FILE_IN="p4_output_"$CLANG_STATUS".tar.gz"
FILE_OUT="hamilton8_np_"$N_PART"_clang_$CLANG_STATUS"

# Run post-processing script
python3 $SCRIPT $DIR_IN$FILE_IN \
--max-cores-per-rank $MAX_CORES_PER_RANK \
--labels "$"{$LABELS}"$" \
--output $DIR_OUT$FILE_OUT \
--log-x \
--export-data
