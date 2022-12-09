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

# First we build the core. This is then drawn upon by the subsequent ReFrame call which issues many tests.

${REFRAME_ROOT:=~}/reframe/bin/reframe \
    --skip-sanity-check \
    --max-retries=0 \
    --exec-policy async \
    --stage $STAGE_DIR \
    --purge-env \
    -C $config \
    -c build_peano_core.py \
    -r \
    -v \
    --performance-report \
    -n \
    'Build_peano_core' \
    --output $OUTPUT_DIR \
    --report-file=build_peano_core.log

${REFRAME_ROOT:=~}/reframe/bin/reframe \
    --max-retries=0 \
    --exec-policy async \
    --stage $STAGE_DIR \
    --purge-env \
    -C $config \
    -c euler.py \
    -c euler_maqao.py \
    -r \
    -v \
    --performance-report \
    -n \
    'Euler' \
    --output $OUTPUT_DIR \
    --report-file=euler.log

