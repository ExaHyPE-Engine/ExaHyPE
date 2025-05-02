#!/usr/bin/env python3

"""
This file contains all possible realisation variants for a swift2 project.
"""

Parallelisation_DomainDecomposition = "domain-decomposition"
Parallelisation_TaskGraph = "task-graph"
Parallelisation_MultisweepTaskGraph = "multisweep-task-graph"
ParallelisationVariants = [
    Parallelisation_DomainDecomposition,
    Parallelisation_TaskGraph,
    Parallelisation_MultisweepTaskGraph,
]

Storage_Scattered = "scattered"
Storage_ContinuousPerVertex = "continuous-per-vertex"
Storage_GlobalContinuous = "global-continuous"
StorageVariants = [
    Storage_Scattered,
    Storage_ContinuousPerVertex,
    Storage_GlobalContinuous,
]

Sorting_MultiscaleSort = "multiscale-sort"
Sorting_BucketSort = "bucket-sort"
SortingVariants = [Sorting_MultiscaleSort, Sorting_BucketSort]

KernelOptimisation_NoOptimisation = "no-optimisation"
KernelOptimisation_VectoriseAll = "vectorise-all"
KernelOptimisation_VectoriseDistanceChecks = "vectorise-distance-checks-preamble"

BasicKernelOptimisationVariants = [
    KernelOptimisation_NoOptimisation,
]
AllKernelOptimisationVariants = [
    KernelOptimisation_NoOptimisation,
    KernelOptimisation_VectoriseAll,
    KernelOptimisation_VectoriseDistanceChecks,
]
