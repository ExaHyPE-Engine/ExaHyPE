#!/bin/bash

libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
./configure --enable-exahype --enable-loadbalancing --enable-blockstructured --with-multithreading=omp \
CC=nvc++ CXX=nvc++
CXXFLAGS="-O4 -std=c++20 -W -Wall -Wextra" \
