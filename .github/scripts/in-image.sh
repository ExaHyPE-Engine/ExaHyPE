#!/bin/bash
# Runs a command in the checkout inside an image. The first call of a job pulls the image.
# Usage: GHCR_TOKEN=<token> .github/scripts/in-image.sh <image reference> <command>

set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <image reference> <command>" >&2
  exit 2
fi

image="$1"
if ! docker image inspect "${image}" >/dev/null 2>&1; then
  docker login ghcr.io --username "${GITHUB_ACTOR}" --password-stdin <<<"${GHCR_TOKEN}" >/dev/null
  docker pull --quiet "${image}"
fi

Variables=(
  CI_COMMIT_REF_NAME PARALLEL_LAUNCH CMAKE_C_COMPILER_LAUNCHER CMAKE_CXX_COMPILER_LAUNCHER
  CMAKE_CUDA_COMPILER_LAUNCHER CMAKE_HIP_COMPILER_LAUNCHER CCACHE_DIR CCACHE_BASEDIR CCACHE_NOHASHDIR CCACHE_MAXSIZE
  ASAN_OPTIONS UBSAN_OPTIONS OMPI_MCA_pml OMPI_MCA_osc OMPI_MCA_btl OMPI_MCA_memory UCX_TLS
)
environment=()
for variable in "${Variables[@]}"; do
  if [ -n "${!variable:-}" ]; then
    environment+=(--env "${variable}")
  fi
done

# The entrypoint of an image sets CC, CXX and FC to the compilers of the image.
compilers=()
for variable in CC CXX; do
  if [ -n "${!variable:-}" ]; then
    compilers+=("${variable}=${!variable}")
  fi
done

docker run --rm --ipc=host --network=host --volume "${PWD}:${PWD}" --workdir "${PWD}" "${environment[@]}" \
  "${image}" env "${compilers[@]}" bash -c "$2"
