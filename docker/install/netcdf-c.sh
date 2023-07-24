#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

# https://downloads.unidata.ucar.edu/netcdf/
NETCDF_C_VERSION=4.9.2
wget --progress=bar:force:noscroll https://github.com/Unidata/netcdf-c/archive/refs/tags/v${NETCDF_C_VERSION}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/v${NETCDF_C_VERSION}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /netcdf-c-${NETCDF_C_VERSION}

CFLAGS="-fPIC" CC=h5pcc ./configure --prefix=${INSTALL_DIR} \
  --disable-dap \
  --enable-shared=no
make -j$(nproc)
#make -j$(nproc) check
make -j$(nproc) install

rm -rf /netcdf-c*
