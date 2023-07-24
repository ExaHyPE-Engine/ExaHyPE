#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

PNETCDF_VERSION=1.12.1
wget --progress=bar:force:noscroll https://parallel-netcdf.github.io/Release/pnetcdf-${PNETCDF_VERSION}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/pnetcdf-${PNETCDF_VERSION}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /pnetcdf-${PNETCDF_VERSION}

CFLAGS="-fPIC" CXXFLAGS="-fPIC" FCFLAGS="-fPIC" CC=mpicc CXX=mpicxx FC=mpif90 ./configure --prefix=${INSTALL_DIR}
make -j$(nproc)
#make -j$(nproc) tests
#make -j$(nproc) check
#make -j$(nproc) ptest
#make -j$(nproc) ptests
make -j$(nproc) install

rm -rf /pnetcdf*
