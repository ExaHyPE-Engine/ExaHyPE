#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

git clone https://github.com/KarypisLab/GKlib.git /GKlib
cd /GKlib
make config prefix=${INSTALL_DIR}
make -j$(nproc)
make install
cd / && rm -rf /GKlib

git clone https://github.com/KarypisLab/METIS.git /METIS
cd /METIS
make config prefix=${INSTALL_DIR}
make -j$(nproc)
make install
cd / && rm -rf /METIS

git clone https://github.com/KarypisLab/ParMETIS.git /ParMETIS
cd /ParMETIS
CC=mpicc CXX=mpicxx make config prefix=${INSTALL_DIR}
make -j$(nproc)
make install
cd / && rm -rf /ParMETIS
