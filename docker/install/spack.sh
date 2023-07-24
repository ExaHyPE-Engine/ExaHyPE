#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

curl -s -L https://api.github.com/repos/spack/spack/tarball | tar xzC $INSTALL_DIR --strip 1
spack -L install \
  environment-modules target=x86_64 \
  lmod target=x86_64 \
  openmpi target=x86_64 \
  eigen target=x86_64 \
  memkind target=x86_64 \
  openblas  target=x86_64 \
  blis threads=openmp target=x86_64 \
  metis +int64 target=x86_64 \
  parmetis +int64 target=x86_64 \
  libxsmm +generator target=x86_64 \
  hdf5 +fortran +threadsafe +mpi target=x86_64 \
  parallel-netcdf +cxx +fortran target=x86_64 \
  netcdf-c +mpi target=x86_64 \
  netcdf-cxx4 target=x86_64 \
  netcdf-fortran target=x86_64
spack -L clean -a
echo "source $(spack location -i lmod)/lmod/lmod/init/bash" >> /etc/profile.d/spack.sh
echo "source $(spack location -i environment-modules)/init/bash" >> /etc/profile.d/spack.sh
echo "source $INSTALL_DIR/share/spack/setup-env.sh" >> /etc/profile.d/spack.sh
rm -rf /tmp/root/spack-stage/
