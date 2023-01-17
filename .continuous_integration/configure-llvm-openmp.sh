#!/bin/bash

read -p "Enter your GPU compute capability
    (possible values are: sm_50, sm_52, sm_53, sm_60, sm_61, sm_62, sm_70, sm_72, sm_75, sm_80, sm_86 or sm_90): " cc

libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
./configure --enable-exahype --enable-loadbalancing --enable-blockstructured --with-gpu=omp --with-multithreading=omp && \
CC=clang CXX=clang++ && \
CXXFLAGS="-std=c++17 -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=$cc"
