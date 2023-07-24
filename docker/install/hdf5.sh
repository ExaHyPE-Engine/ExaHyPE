#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

HDF5_VERSION_MAJOR=1.14
HDF5_VERSION_MINOR=0
wget --progress=bar:force:noscroll https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-${HDF5_VERSION_MAJOR}/hdf5-${HDF5_VERSION_MAJOR}.${HDF5_VERSION_MINOR}/src/hdf5-${HDF5_VERSION_MAJOR}.${HDF5_VERSION_MINOR}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/hdf5-${HDF5_VERSION_MAJOR}.${HDF5_VERSION_MINOR}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /hdf5-${HDF5_VERSION_MAJOR}.${HDF5_VERSION_MINOR}

CFLAGS="-fPIC" CXXFLAGS="-fPIC" FCFLAGS="-fPIC" CC=mpicc CXX=mpicxx FC=mpif90 ./configure --prefix=${INSTALL_DIR} \
  --enable-cxx \
  --enable-fortran \
  --enable-build-mode=production \
  --enable-optimization=high \
  --enable-parallel \
  --enable-tools \
  --enable-unsupported \
  --with-zlib
make -j$(nproc)
#make -j$(nproc) check
make -j$(nproc) install

rm -rf /hdf5*
