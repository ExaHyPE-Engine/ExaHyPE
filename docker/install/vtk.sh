#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

# https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/dev/build.md
git clone --recursive https://gitlab.kitware.com/vtk/vtk.git /vtk
cd /vtk && mkdir build && cd build

cmake -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DVTK_WRAP_PYTHON=ON \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  ..
ninja -j$(nproc)
ninja -j$(nproc) install

rm -rf /vtk
