#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)


git clone --recursive https://github.com/OpenSYCL/OpenSYCL.git /OpenSYCL
cd /OpenSYCL

#git submodule update --init --recursive && \
#mkdir build && cd build && \
#-DCMAKE_BUILD_TYPE:STRING=Release \
#-DCMAKE_INSTALL_PREFIX=/usr/local \
#-DWITH_CPU_BACKEND:Bool=TRUE \
#-DWITH_ROCM_BACKEND:Bool=FALSE \
#-DWITH_CUDA_BACKEND:Bool=TRUE \
#-DLLVM_DIR:String=/usr/local/lib/cmake/llvm \
#-DCLANG_INCLUDE_PATH:String=/usr/local/lib/clang/11.0.1/include \
#-DCLANG_EXECUTABLE_PATH:String=/usr/local/bin/clang++ \
#-DCUDA_TOOLKIT_ROOT_DIR:String=/usr/local/cuda && \
#make -j $(nproc) && make install && cd ../.. && rm -rf ./hipsycl

#rm -rf /apptainer
