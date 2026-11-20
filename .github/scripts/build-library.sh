#!/bin/bash
# Configures, builds, tests and installs the library inside a toolchain image.
# Usage: .github/scripts/build-library.sh

set -euo pipefail

echo "Running CI with $(nproc) cores"
echo "CC=${CC} CXX=${CXX} FC=${FC}"
echo "BUILD_MODE=${BUILD_MODE} SPACE_FILLING_CURVE=${SPACE_FILLING_CURVE} SANITIZER=${SANITIZER}"
echo "GPU_BACKEND=${GPU_BACKEND} GPU_ARCH=${GPU_ARCH} RUN_UNIT_TESTS=${RUN_UNIT_TESTS}"

read -r -a extraOptions <<<"${EXTRA_CONFIGURATION_OPTIONS:-}"

mkdir -p build
cd build
cmake .. -GNinja -DBUILD_TESTING=ON -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON -DPEANO_WITH_FPE=ON \
  -DPEANO_WITH_HDF5=ON -DPEANO_WITH_NETCDF=ON "${extraOptions[@]}" \
  -DSUPPORTED_BUILD_TYPES="${BUILD_MODE}" -DPEANO_WITH_SANITIZER="${SANITIZER}" \
  -DPEANO_WITH_SPACE_FILLING_CURVE="${SPACE_FILLING_CURVE}" -DPEANO_WITH_GPU_BACKEND="${GPU_BACKEND}" \
  -DPEANO_WITH_GPU_ARCH="${GPU_ARCH}" -DCMAKE_INSTALL_PREFIX=../install
ccache --zero-stats
ninja -j "$(nproc)"
ccache --show-stats
if [ "${RUN_UNIT_TESTS}" = true ]; then
  ctest --output-on-failure --timeout 1800
fi
ninja install
