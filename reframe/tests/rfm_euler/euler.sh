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
    "bluefield101.pri.cosma7.alces.network" | "login8a.pri.cosma7.alces.network" | "b101.pri.cosma7.alces.network")
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


${REFRAME_ROOT:=~}/reframe/bin/reframe \
    --max-retries=0 \
    --exec-policy async \
    --stage $STAGE_DIR \
    --purge-env \
    -C $config \
    -c euler.py \
    -r \
    -v \
    --performance-report \
    -n \
    'Euler' \
    --output $OUTPUT_DIR \
    --report-file=euler.log


# If one of our examples takes significantly longer to run than
# previously. ReFrame will make a 'doprofilejob_euler' file in this
# file's directory. If it exists we will immediately delete it 
# (so the check works for future runs of this example) and rerun an
# Euler test with profiler.

FILE=doprofilejob_euler
if test -f "$FILE"; then
    rm "$FILE"
    echo "Following up with profiler for Euler"
    ${REFRAME_ROOT:=~}/reframe/bin/reframe \
        --skip-sanity-check \
        --max-retries=0 \
        --exec-policy async \
        --stage $STAGE_DIR \
        --purge-env \
        -C $config \
        -c euler_maqao.py \
        -r \
        -v \
        --performance-report \
        -n \
        'Euler' \
        --output $OUTPUT_DIR \
        --report-file=euler_maqao.log
fi
