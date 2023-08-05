#!/bin/bash

set -e

INSTALL_DIR="/opt/llvm"
TMP_DIR=$(mktemp -d)

# https://llvm.org/docs/NVPTXUsage.html
# https://llvm.org/docs/AMDGPUUsage.html

git clone https://github.com/llvm/llvm-project.git /llvm-project -b llvmorg-17.0.0-rc1

mkdir /llvm-project/build && cd /llvm-project/build
cmake -GNinja \
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld;polly;openmp" \
  -DLLVM_TARGETS_TO_BUILD="X86;NVPTX" \
  -DLLVM_ENABLE_RUNTIMES="compiler-rt" \
  -DLIBOMPTARGET_NVPTX_COMPUTE_CAPABILITIES="70;72;75;80;86;89;90" \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  ../llvm
ninja -j$(nproc) && ninja -j$(nproc) check-clang

mkdir /llvm-project/build_bootstrap && cd /llvm-project/build_bootstrap
CC=/llvm-project/build/bin/clang CXX=/llvm-project/build/bin/clang++ cmake -GNinja \
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld;polly;openmp" \
  -DLLVM_TARGETS_TO_BUILD="X86;NVPTX" \
  -DLLVM_ENABLE_RUNTIMES="compiler-rt" \
  -DLIBOMPTARGET_NVPTX_COMPUTE_CAPABILITIES="70;72;75;80;86;89;90" \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  ../llvm
ninja -j$(nproc) && ninja -j$(nproc) check-clang && ninja -j$(nproc) install

cd / && rm -rf /llvm-project
