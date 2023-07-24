#!/bin/bash

set -e

INSTALL_DIR="/usr"
TMP_DIR=$(mktemp -d)

# https://github.com/intel/llvm.git