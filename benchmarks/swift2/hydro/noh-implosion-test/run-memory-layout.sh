#!/bin/sh

#
# Ensure that script terminates if one run fails
#
set -e 

for REALISATION in domain-decomposition.scattered.sort-on-the-fly.no-optimisation domain-decomposition.scattered.sort-on-the-fly.outer-guards \
                   domain-decomposition.scattered.insert-sorts.no-optimisation domain-decomposition.scattered.insert-sorts.outer-guards \
                   domain-decomposition.continuous-per-vertex.sort-on-the-fly.no-optimisation domain-decomposition.continuous-per-vertex.sort-on-the-fly.outer-guards \
                   domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-all domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-distance-checks \
                   domain-decomposition.continuous-per-vertex.insert-sorts.no-optimisation domain-decomposition.continuous-per-vertex.insert-sorts.outer-guards \
                   domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-all domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-distance-checks \
                   domain-decomposition.global-continuous.sort-on-the-fly.no-optimisation domain-decomposition.global-continuous.sort-on-the-fly.outer-guards \
                   domain-decomposition.global-continuous.sort-on-the-fly.vectorise-all domain-decomposition.global-continuous.sort-on-the-fly.vectorise-distance-checks \
                   domain-decomposition.global-continuous.insert-sorts.no-optimisation domain-decomposition.global-continuous.insert-sorts.outer-guards \
                   domain-decomposition.global-continuous.insert-sorts.vectorise-all domain-decomposition.global-continuous.insert-sorts.vectorise-distance-checks
do
  EXECUTABLE=noh2D.$REALISATION
  echo $EXECUTABLE
  ./$EXECUTABLE > $EXECUTABLE"-"$OMP_NUM_THREADS"-threads.out"
done

