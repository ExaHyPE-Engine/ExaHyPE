#!/bin/bash
# Installs the compilers and libraries of ROCm 7.2.3 from the package repository of AMD into /opt/rocm-7.2.3.
# Usage: utilities/spack/compilers/amd/install-rocm.sh

set -euo pipefail

mkdir -p /etc/apt/keyrings
wget -qO- https://repo.radeon.com/rocm/rocm.gpg.key | gpg --dearmor >/etc/apt/keyrings/rocm.gpg
echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/rocm.gpg] https://repo.radeon.com/rocm/apt/7.2.3 noble main" \
  >/etc/apt/sources.list.d/rocm.list
# Ubuntu ships older packages of ROCm under the same names.
printf 'Package: *\nPin: origin repo.radeon.com\nPin-Priority: 1001\n' >/etc/apt/preferences.d/rocm
apt-get update -y
apt-get install -y --no-install-recommends rocm-llvm hip-dev hipcc hsa-rocr-dev openmp-extras-dev rocthrust-dev
rm -rf /var/lib/apt/lists/*
