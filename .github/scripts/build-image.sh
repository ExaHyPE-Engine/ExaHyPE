#!/bin/bash
# Builds utilities/docker/Dockerfile.<image> and pushes it to every prefix of IMAGE_PREFIXES.
# Usage: IMAGE_PREFIXES="ghcr.io/exahype-engine exahype" .github/scripts/build-image.sh <image>

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <image>" >&2
  exit 2
fi

image="$1"
repositoryOwner="${GITHUB_REPOSITORY_OWNER:-}"
read -r -a prefixes <<<"${IMAGE_PREFIXES:-ghcr.io/${repositoryOwner,,}}"

if [ "${#prefixes[@]}" -eq 0 ] || [[ "${prefixes[0]}" == */ ]]; then
  echo "Set IMAGE_PREFIXES or GITHUB_REPOSITORY_OWNER." >&2
  exit 2
fi

branch="${GITHUB_REF_NAME:-$(git -C "$(dirname "$0")" rev-parse --abbrev-ref HEAD)}"
if [ "${branch}" = main ]; then
  tag=latest
else
  tag="branch-${branch//[^A-Za-z0-9_.-]/-}"
fi

# Prints the registry of a prefix by the rule of Docker.
registryOf() {
  local registry="${1%%/*}"
  if [[ "$1" != */* || ("${registry}" != *[.:]* && "${registry}" != localhost) ]]; then
    registry=docker.io
  fi
  echo "${registry}"
}

declare -A loggedIn=()
for prefix in "${prefixes[@]}"; do
  registry=$(registryOf "${prefix}")
  if [ -n "${loggedIn[${registry}]:-}" ]; then
    continue
  fi
  loggedIn["${registry}"]=true
  case "${registry}" in
    ghcr.io) username="${GHCR_USERNAME:-}" token="${GHCR_TOKEN:-}" ;;
    docker.io) username="${DOCKERHUB_USERNAME:-}" token="${DOCKERHUB_TOKEN:-}" ;;
    *) username="" token="" ;;
  esac
  if [ -n "${token}" ]; then
    docker login "${registry}" --username "${username}" --password-stdin <<<"${token}"
  fi
done

cd "$(dirname "$0")/../.."

tags=()
for prefix in "${prefixes[@]}"; do
  tags+=(--tag "${prefix}/${image}:${tag}")
done

# Spack pushes every package it installs to this build cache and installs from it.
cacheArguments=()
if [ -n "${GHCR_TOKEN:-}" ] && [ -n "${repositoryOwner}" ]; then
  cacheArguments=(
    --build-arg "SPACK_BUILDCACHE=oci://ghcr.io/${repositoryOwner,,}/spack-buildcache"
    --secret id=spack_buildcache_user,env=GHCR_USERNAME
    --secret id=spack_buildcache_token,env=GHCR_TOKEN
  )
fi

docker build \
  --build-arg "IMAGE_PREFIX=${prefixes[0]}" \
  --build-arg "IMAGE_TAG=${tag}" \
  --label "org.opencontainers.image.source=https://github.com/${GITHUB_REPOSITORY:-ExaHyPE-Engine/ExaHyPE}" \
  "${tags[@]}" \
  "${cacheArguments[@]}" \
  --file "utilities/docker/Dockerfile.${image}" \
  .

for prefix in "${prefixes[@]}"; do
  docker push "${prefix}/${image}:${tag}"
done
