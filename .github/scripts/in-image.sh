#!/bin/bash
# Runs a command in the checkout inside an image of ghcr.io, with the tag IMAGE_TAG or else latest.
# Usage: IMAGE_TAG=<tag> GHCR_TOKEN=<token> .github/scripts/in-image.sh <image> <command>

set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <image> <command>" >&2
  exit 2
fi

repository="ghcr.io/${GITHUB_REPOSITORY_OWNER,,}/$1"
if [ -n "${GHCR_TOKEN:-}" ]; then
  docker login ghcr.io --username "${GITHUB_ACTOR}" --password-stdin <<<"${GHCR_TOKEN}" >/dev/null
fi
image="${repository}:${IMAGE_TAG:-latest}"
if ! docker pull --quiet "${image}"; then
  image="${repository}:latest"
  docker pull --quiet "${image}"
fi
echo "Image: ${image}"

Variables=(
  CI_COMMIT_REF_NAME CC CXX FC BUILD_MODE SPACE_FILLING_CURVE SANITIZER GPU_BACKEND GPU_ARCH RUN_UNIT_TESTS
  RUN_SIMULATIONS EXTRA_CONFIGURATION_OPTIONS MIN_GRID_LEVELS MAX_GRID_LEVELS CMAKE_C_COMPILER_LAUNCHER
  CMAKE_CXX_COMPILER_LAUNCHER CMAKE_CUDA_COMPILER_LAUNCHER CMAKE_HIP_COMPILER_LAUNCHER CCACHE_DIR CCACHE_BASEDIR
  CCACHE_NOHASHDIR CCACHE_MAXSIZE ASAN_OPTIONS UBSAN_OPTIONS OMPI_MCA_pml OMPI_MCA_osc OMPI_MCA_btl OMPI_MCA_memory
  UCX_TLS SYCL_CACHE_PERSISTENT LIBOMPTARGET_PLUGIN LIBOMPTARGET_DEVICETYPE
)
environment=()
for variable in "${Variables[@]}"; do
  if [ -n "${!variable:-}" ]; then
    environment+=(--env "${variable}")
  fi
done

# The entrypoint of an image activates its Spack environment, which sets CC, CXX and FC to the compilers of the
# image. The command sets the compilers of the job again after the activation.
compilers=()
for variable in CC CXX FC; do
  if [ -n "${!variable:-}" ]; then
    compilers+=("${variable}=${!variable}")
  fi
done

docker run --rm --ipc=host --network=host --volume "${PWD}:${PWD}" --workdir "${PWD}" "${environment[@]}" \
  "${image}" env "${compilers[@]}" bash -c "$2"
