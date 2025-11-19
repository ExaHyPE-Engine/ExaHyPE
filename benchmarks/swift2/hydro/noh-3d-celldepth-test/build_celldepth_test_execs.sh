#!/bin/bash

# Set path
export PYTHONPATH=../../../../python

# Script to run experiment (Noh 3D)

# Parameters
END_TIME=0.025 # 25 steps
TIMESTEP_SIZE=0.0001
CFL_FACTOR=0.2
PLOT_DELTA=0.01

# if "false", will skip compiling if exec already exists, and
# will skip running experiment if outputfile already exists
# REDO="false"
REDO="true"

for N_PART in 512 256 128 64 32; do
  for CELL_SIZE_MAX in 2 0.67 0.23; do
    for CELL_SIZE_MIN in 2 0.67 0.23 0.075 0.025 0.0083 0.003; do

      # don't try to build with cell_min > cell_max
      if (( $(echo "$CELL_SIZE_MIN > $CELL_SIZE_MAX" |bc -l) )); then
        continue;
      fi

      SUFFIX="N$N_PART-$CELL_SIZE_MAX-$CELL_SIZE_MIN"
      OUTFILE=output_$SUFFIX.log
      EXEC=noh3D-$SUFFIX

      if [[ "$REDO" == "true" ]] || [[ ! -f "$EXEC" ]]; then

        # Run python script to generate the experiment
        echo COMPILING N_PART=$N_PART CELL_MAX=$CELL_SIZE_MAX CELL_MIN=$CELL_SIZE_MIN
        python3 noh.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA --cell-size-max $CELL_SIZE_MAX --cell-size-min $CELL_SIZE_MIN -j 64 > /dev/null
        # allow this to fail if we're trying to use too small cell_size_min. The python script will detect it.

        if [ -f noh3D ]; then
            # rename exec
            mv noh3D $EXEC
        fi
      fi

    done
  done
done


echo 'Done. Bye!'
