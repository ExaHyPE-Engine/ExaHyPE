#!/bin/bash
# Prints the images to build for the images given or for the changed files on standard input.
# Usage: plan-images.sh <branch> [<image>...]

set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <branch> [<image>...]" >&2
  exit 2
fi

branch="$1"
shift

Toolchains=(gcc llvm intel nvidia amd exahype)
Images=(base spack "${Toolchains[@]}" tutorials)

# Prints the files and folders that an image is built from. A folder ends with a slash.
sources() {
  local spackPackages="utilities/spack/repo.yaml utilities/spack/packages/"
  case "$1" in
    base)
      echo "utilities/docker/Dockerfile.base .github/workflows/image-build.yml .github/scripts/build-image.sh"
      echo ".github/scripts/plan-images.sh"
      ;;
    spack) echo "utilities/docker/Dockerfile.spack" ;;
    gcc | nvidia) echo "utilities/docker/Dockerfile.$1 utilities/spack/compilers/$1/ requirements.txt" ;;
    llvm | intel | amd)
      echo "utilities/docker/Dockerfile.$1 utilities/spack/compilers/$1/ requirements.txt ${spackPackages}"
      ;;
    exahype)
      echo "utilities/docker/Dockerfile.exahype utilities/spack/repo.yaml utilities/spack/packages/exahype2/"
      ;;
    tutorials) echo "utilities/docker/Dockerfile.tutorials" ;;
    *)
      echo "Unknown image $1" >&2
      return 1
      ;;
  esac
}

# Prints the image that an image starts from.
parent() {
  case "$1" in
    base) ;;
    spack) echo base ;;
    tutorials) echo exahype ;;
    *) echo spack ;;
  esac
}

declare -A selected=()

if [ "$#" -gt 0 ]; then
  for image in "$@"; do
    sources "${image}" >/dev/null
    selected["${image}"]=true
  done
else
  while read -r file || [ -n "${file}" ]; do
    for image in "${Images[@]}"; do
      for source in $(sources "${image}"); do
        if [[ "${file}" == "${source}" || ("${source}" == */ && "${file}" == "${source}"*) ]]; then
          selected["${image}"]=true
        fi
      done
    done
  done
  # Selects every image built on a selected image. Images lists parents first.
  for image in "${Images[@]}"; do
    parentImage=$(parent "${image}")
    if [ -n "${parentImage}" ] && [ -n "${selected[${parentImage}]:-}" ]; then
      selected["${image}"]=true
    fi
  done
fi

if [ "${branch}" != main ]; then
  for ((index = ${#Images[@]} - 1; index >= 0; index--)); do
    image="${Images[index]}"
    parentImage=$(parent "${image}")
    if [ -n "${parentImage}" ] && [ -n "${selected[${image}]:-}" ]; then
      selected["${parentImage}"]=true
    fi
  done
fi

toolchains=()
for image in "${Toolchains[@]}"; do
  if [ -n "${selected[${image}]:-}" ]; then
    toolchains+=("\"${image}\"")
  fi
done

echo "base=${selected[base]:-false}"
echo "spack=${selected[spack]:-false}"
echo "toolchains=[$(
  IFS=,
  echo "${toolchains[*]}"
)]"
echo "tutorials=${selected[tutorials]:-false}"
