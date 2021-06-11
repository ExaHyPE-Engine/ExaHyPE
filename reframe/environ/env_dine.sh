#!/bin/bash
module purge
module load python/3.6.5

export STAGE_DIR=/cosma5/data/durham/$USER/reframe_stage
export OUTPUT_DIR=/cosma5/data/durham/$USER/reframe_output
export RFM_SAVE_LOG_FILES=1

export PYTHONPATH=$PYTHONPATH:~/Peano/reframe/tests
