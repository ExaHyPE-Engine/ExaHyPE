#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

git clone --recursive https://gitlab.com/libeigen/eigen.git /eigen
cd /eigen && mkdir build && cd build

cmake -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  ..
ninja -j$(nproc)
#ninja -j$(nproc) check
ninja -j$(nproc) install

rm -rf /eigen
