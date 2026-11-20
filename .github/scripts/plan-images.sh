#!/bin/bash
# Prints the images to build: the images given, else every image whose published label differs from its sources.
# Usage: [IMAGE_PREFIX=<prefix>] plan-images.sh <branch> [<image>...] | plan-images.sh --hash <image>

set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <branch> [<image>...] | $0 --hash <image>" >&2
  exit 2
fi

Toolchains=(gcc llvm intel nvidia amd exahype)
declare -A DisplayNames=([gcc]=GCC [llvm]=LLVM [intel]=Intel [nvidia]=NVIDIA [amd]=AMD [exahype]=ExaHyPE)
Images=(base spack "${Toolchains[@]}" tutorials)

# Prints the files and folders that an image is built from. A folder ends with a slash.
sources() {
  local spackPackages="utilities/spack/repo.yaml utilities/spack/packages/"
  case "$1" in
    base) echo "utilities/docker/Dockerfile.base" ;;
    spack) echo "utilities/docker/Dockerfile.spack" ;;
    gcc | nvidia) echo "utilities/docker/Dockerfile.$1 utilities/spack/compilers/$1/ requirements.txt" ;;
    llvm | intel | amd)
      echo "utilities/docker/Dockerfile.$1 utilities/spack/compilers/$1/ requirements.txt ${spackPackages}"
      ;;
    exahype)
      echo "utilities/docker/Dockerfile.exahype utilities/spack/repo.yaml utilities/spack/packages/exahype2/"
      echo "CMakeLists.txt cmake/ src/ api/ requirements.txt tests/ tutorials/ applications/"
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

# Prints the hash of the committed sources of an image and of its parents.
hashOf() {
  local parentImage
  parentImage=$(parent "$1")
  {
    if [ -n "${parentImage}" ]; then
      hashOf "${parentImage}"
    fi
    # shellcheck disable=SC2046
    git -C "$(dirname "$0")" ls-tree -r HEAD -- $(sources "$1" | sed "s|[^ ]*|:/&|g")
  } | sha256sum | cut -c1-16
}

if [ "$1" = --hash ]; then
  hashOf "$2"
  exit 0
fi

branch="$1"
shift
tag=latest
if [ "${branch}" != main ]; then
  tag="branch-${branch//[^A-Za-z0-9_.-]/-}"
fi

# Returns whether IMAGE_PREFIX contains the image with the tag of the branch.
published() {
  [ -n "${IMAGE_PREFIX:-}" ] && docker buildx imagetools inspect "${IMAGE_PREFIX}/$1:${tag}" >/dev/null 2>&1
}

# Prints the sources label of the image of the branch, else of the image of main.
publishedHash() {
  local reference
  for reference in "${IMAGE_PREFIX}/$1:${tag}" "${IMAGE_PREFIX}/$1:latest"; do
    if docker buildx imagetools inspect --format '{{json .Image}}' "${reference}" 2>/dev/null |
      jq -r '.config.Labels["org.exahype.image.sources"] // ""'; then
      return
    fi
  done
}

declare -A selected=()

if [ "$#" -gt 0 ]; then
  for image in "$@"; do
    sources "${image}" >/dev/null
    selected["${image}"]=true
  done
elif [ -n "${IMAGE_PREFIX:-}" ]; then
  for image in "${Images[@]}"; do
    if [ "$(publishedHash "${image}")" != "$(hashOf "${image}")" ]; then
      selected["${image}"]=true
    fi
  done
fi

if [ "${branch}" != main ]; then
  for ((index = ${#Images[@]} - 1; index >= 0; index--)); do
    image="${Images[index]}"
    parentImage=$(parent "${image}")
    if [ -n "${parentImage}" ] && [ -n "${selected[${image}]:-}" ] && [ -z "${selected[${parentImage}]:-}" ] &&
      ! published "${parentImage}"; then
      selected["${parentImage}"]=true
    fi
  done
fi

toolchains=()
for image in "${Toolchains[@]}"; do
  if [ -n "${selected[${image}]:-}" ]; then
    toolchains+=("{\"image\":\"${image}\",\"name\":\"${DisplayNames[${image}]}\"}")
  fi
done

echo "base=${selected[base]:-false}"
echo "spack=${selected[spack]:-false}"
echo "toolchains=[$(
  IFS=,
  echo "${toolchains[*]}"
)]"
echo "tutorials=${selected[tutorials]:-false}"
