#!/bin/sh

#
# Ensure that script terminates if one run fails
#
set -e 

for REALISATION in task-tree.scattered.bucket-sort.no-optimisation task-tree.scattered.bucket-sort.outer-guards \
                   task-tree.scattered.multiscale-sort.no-optimisation task-tree.scattered.multiscale-sort.outer-guards \
                   task-tree.continuous-per-vertex.bucket-sort.no-optimisation task-tree.continuous-per-vertex.bucket-sort.outer-guards \
                   task-tree.continuous-per-vertex.bucket-sort.vectorise-all task-tree.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   task-tree.continuous-per-vertex.multiscale-sort.no-optimisation task-tree.continuous-per-vertex.multiscale-sort.outer-guards \
                   task-tree.continuous-per-vertex.multiscale-sort.vectorise-all task-tree.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   task-tree.global-continuous.bucket-sort.no-optimisation task-tree.global-continuous.bucket-sort.outer-guards \
                   task-tree.global-continuous.bucket-sort.vectorise-all task-tree.global-continuous.bucket-sort.vectorise-distance-checks \
                   task-tree.global-continuous.multiscale-sort.no-optimisation task-tree.global-continuous.multiscale-sort.outer-guards \
                   task-tree.global-continuous.multiscale-sort.vectorise-all task-tree.global-continuous.multiscale-sort.vectorise-distance-checks
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

