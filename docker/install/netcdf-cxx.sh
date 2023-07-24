#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

# https://downloads.unidata.ucar.edu/netcdf/
NETCDF_CXX_VERSION=4.3.1
wget --progress=bar:force:noscroll https://github.com/Unidata/netcdf-cxx4/archive/refs/tags/v${NETCDF_CXX_VERSION}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/v${NETCDF_CXX_VERSION}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /netcdf-cxx4-${NETCDF_CXX_VERSION}

CXXFLAGS="-fPIC" LDFLAGS="-lcurl" CXX=h5c++ ./configure --prefix=${INSTALL_DIR} \
  --enable-shared=no
make -j$(nproc)
#make -j$(nproc) check
make -j$(nproc) install

rm -rf /netcdf-cxx4*
