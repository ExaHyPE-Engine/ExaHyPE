#!/bin/bash

set -e

INSTALL_DIR="/usr"
TMP_DIR=$(mktemp -d)

HWLOC_VERSION_MAJOR=2.9
HWLOC_VERSION_MINOR=1
wget --progress=bar:force:noscroll http://www.open-mpi.org/software/hwloc/v${HWLOC_VERSION_MAJOR}/downloads/hwloc-${HWLOC_VERSION_MAJOR}.${HWLOC_VERSION_MINOR}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/hwloc-${HWLOC_VERSION_MAJOR}.${HWLOC_VERSION_MINOR}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /hwloc-${HWLOC_VERSION_MAJOR}.${HWLOC_VERSION_MINOR}

./configure --prefix=${INSTALL_DIR} \
  --disable-opencl \
  --disable-cairo \
  --disable-nvml \
  --disable-gl \
  --disable-rsmi \
  --disable-levelzero \
  --disable-libudev \
  --enable-shared \
  --enable-cuda
make -j$(nproc)
#make -j$(nproc) check
make -j$(nproc) install

rm -rf /hwloc*
