#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

git clone -b release https://gitlab.com/petsc/petsc.git /petsc
cd /petsc

# https://petsc.org/release/install/install/
PETSC_ARCH=linux-gnu-c-opt ./configure --with-cc=mpicc --with-cxx=mpicxx --with-fc=mpif90 --prefix=${INSTALL_DIR}
make -j$(nproc)
#make -j$(nproc) check
#make -j$(nproc) test
make -j$(nproc) install

cd / && rm -rf /petsc
