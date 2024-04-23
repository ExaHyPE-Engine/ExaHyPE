#!/usr/bin/env bash

rm -rf ./build
cp setup_small.h setup.h
mkdir build
cd build
cmake -DCMAKE_CXX_COMPILER=/bin/clang++ -DCMAKE_BUILD_TYPE=Release ../ && make && cp noh2d ../noh2d-small
cd ..
