#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

LIBXSMM_VERSION=1.16.1
git clone --recursive https://github.com/libxsmm/libxsmm.git /libxsmm
cd /libxsmm
git checkout ${LIBXSMM_VERSION}

make -j$(nproc)
make -j$(nproc) generator
make -j$(nproc) PREFIX=${INSTALL_DIR} install

cd / && rm -rf /libxsmm*
