#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

# https://github.com/apptainer/apptainer/blob/main/INSTALL.md
git clone https://github.com/apptainer/apptainer.git /apptainer -b v1.1.8
cd /apptainer

./mconfig --with-suid --prefix=${INSTALL_DIR}
cd ./builddir
make
make install

rm -rf /apptainer
