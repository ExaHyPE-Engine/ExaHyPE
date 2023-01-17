#!/bin/bash

read -p "Enter your GPU compute capability
    (possible values are: cc50, cc52, cc53, cc60, cc61, cc62, cc70, cc72, cc75, cc80, cc86 or cc90): " cc

libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
./configure --enable-exahype --enable-loadbalancing --enable-blockstructured --with-gpu=cpp --with-multithreading=cpp \
CC=nvc++ CXX=nvc++ \
CXXFLAGS="-std=c++20 -W -Wall -Wextra -mp=gpu -gpu=$cc -cuda" \
LDFLAGS="-mp=gpu -gpu=$cc -cuda"
