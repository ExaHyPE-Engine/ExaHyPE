#!/bin/sh

export PYTHONPATH=$PYTHONPATH:../../../../python

# Script to run experiment (Noh 2D)

# Parameters
TIMESTEP_SIZE=0.0001
END_TIME=0.001
#N_PART=2000
PLOT_DELTA=0.0

rm noh2D.*

for REALISATION in domain-decomposition.scattered.no-optimisation domain-decomposition.scattered.outer-guards
do
  N_PART=240
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION
  N_PART=120
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m asserts -o noh2D.$REALISATION-asserts
done



#for REALISATION in domain-decomposition.scattered.no-optimisation domain-decomposition.scattered.outer-guards \
#                   domain-decomposition.continuous-per-vertex.insert-sorts  domain-decomposition.continuous-per-vertex.sort-on-the-fly \
#                   domain-decomposition.global-continuous.insert-sorts domain-decomposition.global-continuous.sort-on-the-fly \
#                   domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-all  domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-all \
#                   domain-decomposition.global-continuous.insert-sorts.vectorise-all domain-decomposition.global-continuous.sort-on-the-fly.vectorise-all




