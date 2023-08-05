#!/bin/bash

set -e

INSTALL_DIR="/opt/gcc"
TMP_DIR=$(mktemp -d)

# https://gcc.gnu.org/install/specific.html#nvptx-x-none
git clone https://github.com/MentorEmbedded/nvptx-tools/ /nvptx-tools
cd /nvptx-tools && ./configure --prefix=${INSTALL_DIR} && make -j$(nproc) && make -j$(nproc) install

git clone git://gcc.gnu.org/git/gcc.git -b releases/gcc-13.2.0 /gcc && git clone https://github.com/MentorEmbedded/nvptx-newlib/ /nvptx-newlib
cd /gcc && ln -s ../nvptx-newlib/newlib newlib && cd /gcc && contrib/download_prerequisites

mkdir /gcc/build-nvptx-gcc && cd /gcc/build-nvptx-gcc
../configure \
  --target=nvptx-none \
  --enable-as-accelerator-for=x86_64-pc-linux-gnu \
  --with-build-time-tools=${INSTALL_DIR}/bin \
  --disable-sjlj-exceptions \
  --with-newlib \
  --with-arch=sm_70 \
  --enable-newlib-io-long-long \
  --enable-languages=c,c++,fortran,lto \
  --prefix=${INSTALL_DIR} && \
make -j$(nproc) && make -j$(nproc) install

mkdir /gcc/build-host-gcc && cd /gcc/build-host-gcc
../configure \
  --build=x86_64-pc-linux-gnu \
  --host=x86_64-pc-linux-gnu \
  --target=x86_64-pc-linux-gnu \
  --disable-multilib \
  --enable-offload-targets=nvptx-none=${INSTALL_DIR}/usr/local/bin/ \
  --enable-languages=c,c++,fortran,lto \
  --prefix=${INSTALL_DIR} && \
make -j$(nproc) && make -j$(nproc) install

cd / && rm -rf /nvptx-tools && rm -rf /nvptx-newlib && rm -rf /gcc
