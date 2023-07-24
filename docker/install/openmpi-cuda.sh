#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

OPENMPI_VERSION_MAJOR=4.1
OPENMPI_VERSION_MINOR=5
wget --progress=bar:force:noscroll https://download.open-mpi.org/release/open-mpi/v${OPENMPI_VERSION_MAJOR}/openmpi-${OPENMPI_VERSION_MAJOR}.${OPENMPI_VERSION_MINOR}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/openmpi-${OPENMPI_VERSION_MAJOR}.${OPENMPI_VERSION_MINOR}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /openmpi-${OPENMPI_VERSION_MAJOR}.${OPENMPI_VERSION_MINOR}

mkdir ./build
cd ./build

../configure --prefix=${INSTALL_DIR} \
  --with-memory-manager=none \
  --enable-static=yes \
  --enable-mpirun-prefix-by-default \
  --with-hwloc=external \
  --with-cuda
make -j$(nproc)
make -j$(nproc) check
make -j$(nproc) install

rm -rf /openmpi*
