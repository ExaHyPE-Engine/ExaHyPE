#!/bin/bash

./configure --with-multithreading=omp --enable-exahype --enable-loadbalancing --enable-blockstructured CXXFLAGS="-fopenmp -std=c++14 -DnoMPISupportsSingleSidedCommunication"
