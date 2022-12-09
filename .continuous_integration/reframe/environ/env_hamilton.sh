#!/bin/bash
module purge
module load python/3.6.8

echo "Environment file in use: env_hamilton.sh"

export STAGE_DIR=${STAGE_DIR:-'/ddn/data/$USER/reframe_stage'}
export OUTPUT_DIR=${OUTPUT_DIR:-'/ddn/data/$USER/reframe_output'}
export RFM_SAVE_LOG_FILES=1
export config=../../config/hamilton.py

if [ -z ${TESTLIB+x} ]
then
    echo "TESTLIB is unset, fall back to ~/Peano/reframe/tests"
    export PYTHONPATH=$PYTHONPATH:~/Peano/reframe/tests
else
    echo "TESTLIB is set to '$TESTLIB'"
    export PYTHONPATH=$PYTHONPATH:${TESTLIB}
fi

