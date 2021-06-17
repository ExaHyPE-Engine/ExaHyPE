#!/bin/bash
if [ $# -eq 0 ]
then
    echo "Git revision not supplied"
    exit 1
fi
export GIT_REVISION="${1}"

case $HOSTNAME in
    "hamilton1.hpc.dur.ac.uk" | "hamilton2.hpc.dur.ac.uk")
        source ../../environ/env_hamilton.sh
        ;;
    "login8a.pri.cosma7.alces.network" | "b101.pri.cosma7.alces.network")
        source ../../environ/env_dine.sh
        ;;
    "login2.bede.dur.ac.uk")
        source ../../environ/env_bede.sh
        ;;
    (*)
        echo "Working on an unknown system"
        exit 1
        ;;
esac

~/reframe/bin/reframe \
    --max-retries=0 \
    --exec-policy async \
    --stage $STAGE_DIR \
    --purge-env \
    -C $config \
    -c euler_with_smartmpi.py \
    -r \
    -v \
    --performance-report \
    -n \
    'Euler' \
    --output $OUTPUT_DIR \
    --report-file=euler.log

