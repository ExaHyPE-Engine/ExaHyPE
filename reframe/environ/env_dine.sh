#!/bin/bash
module purge
module load python/3.6.5


echo "Environment file in use: env_dine.sh"

export STAGE_DIR=${STAGE_DIR:-'/cosma5/data/durham/$USER/reframe_stage'}
export OUTPUT_DIR=${OUTPUT_DIR:-'/cosma5/data/durham/$USER/reframe_output'}
export RFM_SAVE_LOG_FILES=1
export config=../../config/dine.py

if [ -z ${TESTLIB+x} ]
then
    echo "TESTLIB is unset, fall back to ~/Preano/reframe/tests"
    export PYTHONPATH=$PYTHONPATH:~/Peano/reframe/tests
else 
    echo "TESTLIB is set to '$TESTLIB'"
    export PYTHONPATH=$PYTHONPATH:${TESTLIB}
fi
