#!/bin/bash

export CLAW=/ddn/data/cvxs42/clawpack_src/clawpack-v5-8-0
export PYTHONPATH=/ddn/data/cvxs42/Peano/python

module purge
module load gsl/intel/2.4
module load intel/2020.4 gcc/9.3.0 intelmpi/intel/2019.6 python/3.6.8
module unload gcc/8.2.0


