#!/bin/sh

#
# Ensure that script terminates if one run fails
#
set -e 

for REALISATION in task-graph.scattered.bucket-sort.no-optimisation task-graph.scattered.bucket-sort.outer-guards \
                   task-graph.scattered.multiscale-sort.no-optimisation task-graph.scattered.multiscale-sort.outer-guards \
                   task-graph.continuous-per-vertex.bucket-sort.no-optimisation task-graph.continuous-per-vertex.bucket-sort.outer-guards \
                   task-graph.continuous-per-vertex.bucket-sort.vectorise-all task-graph.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   task-graph.continuous-per-vertex.multiscale-sort.no-optimisation task-graph.continuous-per-vertex.multiscale-sort.outer-guards \
                   task-graph.continuous-per-vertex.multiscale-sort.vectorise-all task-graph.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   task-graph.global-continuous.bucket-sort.no-optimisation task-graph.global-continuous.bucket-sort.outer-guards \
                   task-graph.global-continuous.bucket-sort.vectorise-all task-graph.global-continuous.bucket-sort.vectorise-distance-checks \
                   task-graph.global-continuous.multiscale-sort.no-optimisation task-graph.global-continuous.multiscale-sort.outer-guards \
                   task-graph.global-continuous.multiscale-sort.vectorise-all task-graph.global-continuous.multiscale-sort.vectorise-distance-checks
do
  clear
  EXECUTABLE=noh2D.$REALISATION-asserts
  echo $EXECUTABLE
  ./$EXECUTABLE
  
  EXECUTABLE=noh2D.$REALISATION-test
  echo $EXECUTABLE
  ./$EXECUTABLE

  rm *patch-file *.vtu *.pvd
done

