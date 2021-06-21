#!/bin/bash
module purge
# No need to load a python module apparently

echo "Environment file in use: env_bede.sh"

export STAGE_DIR=${STAGE_DIR:-'/nobackup/projects/bddur02/reframe_stage'}
export OUTPUT_DIR=${OUTPUT_DIR:-'/nobackup/projects/bddur02/reframe_output'}
export RFM_SAVE_LOG_FILES=1
export config=../../config/bede.py

export PYTHONPATH=$PYTHONPATH:~/Peano/reframe/tests

