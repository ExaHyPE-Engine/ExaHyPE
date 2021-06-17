#!/bin/bash
module purge
module load python/3.6.8

echo "Environment file in use: env_hamilton.sh"

export STAGE_DIR=${STAGE_DIR:-'/ddn/data/$USER/reframe_stage'}
export OUTPUT_DIR=${OUTPUT_DIR:-'/ddn/data/$USER/reframe_output'}
export RFM_SAVE_LOG_FILES=1
export config=../../config/hamilton.py

export PYTHONPATH=$PYTHONPATH:~/Peano/reframe/tests
