#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

OPENBLAS_VERSION=0.3.23
wget --progress=bar:force:noscroll https://github.com/xianyi/OpenBLAS/archive/v${OPENBLAS_VERSION}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/v${OPENBLAS_VERSION}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /OpenBLAS-${OPENBLAS_VERSION}

make
make install PREFIX=${INSTALL_DIR}

rm -rf /OpenBLAS*
