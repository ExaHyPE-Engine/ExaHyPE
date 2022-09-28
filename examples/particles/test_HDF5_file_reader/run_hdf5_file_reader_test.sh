#!/bin/bash

# @TODO include assertion for particle properties.
# @TODO make sure it will use the correct compiler

rm test.out

# Generate project
python3 minimal-sph-peano-sod-shock-1D.py -dist 1e-1 -m assert 

# Make sure h5c++ is used
sed -i 's/g++/h5c++/g' Makefile
make -j8

export OMP_NUM_THREADS=1; ./peano4 > test.out
