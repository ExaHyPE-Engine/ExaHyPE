#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

# https://downloads.unidata.ucar.edu/netcdf/
NETCDF_FORTRAN_VERSION=4.6.0
wget --progress=bar:force:noscroll https://github.com/Unidata/netcdf-fortran/archive/refs/tags/v${NETCDF_FORTRAN_VERSION}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/v${NETCDF_FORTRAN_VERSION}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /netcdf-fortran-${NETCDF_FORTRAN_VERSION}

FCFLAGS="-fPIC" FC="h5pfc" LDFLAGS="$(nc-config --libs --static)" ./configure --prefix=${INSTALL_DIR} \
  --disable-shared
make -j$(nproc)
#make -j$(nproc) check
make -j$(nproc) install

rm -rf /netcdf-fortran*
