#!/bin/sh

#
# Ensure that script terminates if one run fails
#
set -e 

for REALISATION in domain-decomposition.scattered.bucket-sort.no-optimisation domain-decomposition.scattered.bucket-sort.outer-guards \
                   domain-decomposition.scattered.multiscale-sort.no-optimisation domain-decomposition.scattered.multiscale-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.bucket-sort.no-optimisation domain-decomposition.continuous-per-vertex.bucket-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.bucket-sort.vectorise-all domain-decomposition.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   domain-decomposition.continuous-per-vertex.multiscale-sort.no-optimisation domain-decomposition.continuous-per-vertex.multiscale-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.multiscale-sort.vectorise-all domain-decomposition.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   domain-decomposition.global-continuous.bucket-sort.no-optimisation domain-decomposition.global-continuous.bucket-sort.outer-guards \
                   domain-decomposition.global-continuous.bucket-sort.vectorise-all domain-decomposition.global-continuous.bucket-sort.vectorise-distance-checks \
                   domain-decomposition.global-continuous.multiscale-sort.no-optimisation domain-decomposition.global-continuous.multiscale-sort.outer-guards \
                   domain-decomposition.global-continuous.multiscale-sort.vectorise-all domain-decomposition.global-continuous.multiscale-sort.vectorise-distance-checks \
                   task-tree.scattered.bucket-sort.no-optimisation task-tree.scattered.bucket-sort.outer-guards \
                   task-tree.scattered.multiscale-sort.no-optimisation task-tree.scattered.multiscale-sort.outer-guards \
                   task-tree.continuous-per-vertex.bucket-sort.no-optimisation task-tree.continuous-per-vertex.bucket-sort.outer-guards \
                   task-tree.continuous-per-vertex.bucket-sort.vectorise-all task-tree.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   task-tree.continuous-per-vertex.multiscale-sort.no-optimisation task-tree.continuous-per-vertex.multiscale-sort.outer-guards \
                   task-tree.continuous-per-vertex.multiscale-sort.vectorise-all task-tree.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   task-tree.global-continuous.bucket-sort.no-optimisation task-tree.global-continuous.bucket-sort.outer-guards \
                   task-tree.global-continuous.bucket-sort.vectorise-all task-tree.global-continuous.bucket-sort.vectorise-distance-checks \
                   task-tree.global-continuous.multiscale-sort.no-optimisation task-tree.global-continuous.multiscale-sort.outer-guards \
                   task-tree.global-continuous.multiscale-sort.vectorise-all task-tree.global-continuous.multiscale-sort.vectorise-distance-checks \
                   task-graph.scattered.bucket-sort.no-optimisation task-graph.scattered.bucket-sort.outer-guards \
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
  EXECUTABLE=noh2D.$REALISATION
  echo $EXECUTABLE
  ./$EXECUTABLE > $EXECUTABLE"-"$OMP_NUM_THREADS".out"
done

