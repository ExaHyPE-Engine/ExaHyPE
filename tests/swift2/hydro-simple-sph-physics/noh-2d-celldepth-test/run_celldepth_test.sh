#!/bin/bash

# This script is intended for running locally/on an interactive session.

# set -e

# Function to time the process.
# Usage: timing `executable` `args`
timing() {

    timeformat="----------------------------------------------------------------\n"
    timeformat="$timeformat""stats for %C\n"
    timeformat="$timeformat""AverageMem(kb),MaxMem(kb),WallTime(s),CPUTime(s),UserTime(s),CPUPercentage\n"
    timeformat="$timeformat""%K,%M,%e,%S,%U,%P"

    /usr/bin/time -f "$timeformat" "$@"
}


# Set path
export PYTHONPATH=../../../../python

# Script to run experiment (Noh 2D)

# if "false", will skip compiling if exec already exists, and
# will skip running experiment if outputfile already exists
# REDO="false"
REDO="true"

for N_PART in 512 256 128 64 32; do
  for CELL_SIZE_MAX in 2 0.67 0.23; do
    for CELL_SIZE_MIN in 2 0.67 0.23 0.075 0.025 0.0083 0.003; do

      if (( $(echo "$CELL_SIZE_MIN > $CELL_SIZE_MAX" |bc -l) )); then
        continue;
      fi

      SUFFIX="N$N_PART-$CELL_SIZE_MAX-$CELL_SIZE_MIN"
      OUTFILE=output_$SUFFIX.log
      EXEC=noh2D-$SUFFIX

      if [ -f $EXEC ]; then
        if [[ "$REDO" == "true" ]] || [[ ! -f "$OUTFILE" ]]; then

          echo RUNNING N_PART=$N_PART CELL_MAX=$CELL_SIZE_MAX CELL_MIN=$CELL_SIZE_MIN

          timing -o "$OUTFILE" -a ./noh2D-$SUFFIX 2>&1 > "$OUTFILE"
          echo FINISHED N_PART=$N_PART CELL_MAX=$CELL_SIZE_MAX CELL_MIN=$CELL_SIZE_MIN
          tail -n 4 "$OUTFILE"

        else
            echo SKIPPING EXISTING RUN N_PART=$N_PART CELL_MAX=$CELL_SIZE_MAX CELL_MIN=$CELL_SIZE_MIN
        fi

      else
        echo "SKIPPING RUN N_PART=$N_PART CELL_MAX=$CELL_SIZE_MAX CELL_MIN=$CELL_SIZE_MIN : EXEC DOESN'T EXIST"
      fi
    done
  done
done


echo 'Done. Bye!'
