#!/bin/bash
module purge
# No need to load a python module apparently

export STAGE_DIR=/nobackup/projects/bddur02/reframe_stage
export OUTPUT_DIR=/nobackup/projects/bddur02/reframe_output
export RFM_SAVE_LOG_FILES=1

export PYTHONPATH=$PYTHONPATH:~/Peano/reframe/tests

