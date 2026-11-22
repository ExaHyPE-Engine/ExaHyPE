#!/bin/bash
# Builds utilities/docker/Dockerfile.<image> and pushes it to every prefix of IMAGE_PREFIXES, unless the published
# image already contains the sources. Writes the hash of the sources of the image and its parents to GITHUB_OUTPUT.
# Usage: SOURCES="<source>..." .github/scripts/build-image.sh <image> [<hash of the parent image>]

set -euo pipefail

IMAGE="$1"
PARENT_HASH="${2:-}"

read -r -a prefixes <<<"${IMAGE_PREFIXES:-ghcr.io/${GITHUB_REPOSITORY_OWNER,,}}"
read -r -a sources <<<"${SOURCES:?}"
if [ "${IMAGE_BRANCH}" = main ]; then
  tag=latest
else
  tag="branch-${IMAGE_BRANCH//[^A-Za-z0-9_.-]/-}"
fi
# The paths relative to .github/scripts keep the hashes of the published images.
hash=$({
  if [ -n "${PARENT_HASH}" ]; then
    echo "${PARENT_HASH}"
  fi
  git -C .github/scripts ls-tree -r HEAD -- "${sources[@]/#/:/}"
} | sha256sum | cut -c1-16)
echo "hash=${hash}" >>"${GITHUB_OUTPUT}"

for prefix in "${prefixes[@]}"; do
  registry="${prefix%%/*}"
  if [[ "${prefix}" != */* || ("${registry}" != *[.:]* && "${registry}" != localhost) ]]; then
    registry=docker.io
  fi
  case "${registry}" in
    ghcr.io) docker login ghcr.io --username "${GHCR_USERNAME}" --password-stdin <<<"${GHCR_TOKEN}" ;;
    docker.io) docker login docker.io --username "${DOCKERHUB_USERNAME}" --password-stdin <<<"${DOCKERHUB_TOKEN}" ;;
  esac
done

label() {
  docker buildx imagetools inspect --format '{{json .Image}}' "$1" 2>/dev/null |
    jq -r '.config.Labels["org.exahype.image.sources"] // ""'
}
published=$(label "${prefixes[0]}/${IMAGE}:${tag}" || true)
if [ -z "${published}" ]; then
  published=$(label "${prefixes[0]}/${IMAGE}:latest" || true)
fi
if [ -n "${IMAGES}" ] && [[ " ${IMAGES} " != *" ${IMAGE} "* ]]; then
  echo "${IMAGE} is not requested"
  exit 0
elif [ -z "${IMAGES}" ] && [ "${published}" = "${hash}" ]; then
  echo "${IMAGE} is current"
  exit 0
fi

parents=$(grep -oP 'FROM \$\{IMAGE_PREFIX\}/\K[a-z]+' "utilities/docker/Dockerfile.${IMAGE}" | sort -u || true)
for parent in ${parents}; do
  if ! docker buildx imagetools inspect "${prefixes[0]}/${parent}:${tag}" >/dev/null 2>&1; then
    docker buildx imagetools create --tag "${prefixes[0]}/${parent}:${tag}" "${prefixes[0]}/${parent}:latest"
  fi
done

tags=()
for prefix in "${prefixes[@]}"; do
  tags+=(--tag "${prefix}/${IMAGE}:${tag}")
done
docker build \
  --build-arg "IMAGE_PREFIX=${prefixes[0]}" \
  --build-arg "IMAGE_TAG=${tag}" \
  --build-arg "SPACK_BUILDCACHE=oci://ghcr.io/${GITHUB_REPOSITORY_OWNER,,}/spack-buildcache" \
  --secret id=spack_buildcache_user,env=GHCR_USERNAME \
  --secret id=spack_buildcache_token,env=GHCR_TOKEN \
  --label "org.exahype.image.sources=${hash}" \
  --label "org.opencontainers.image.source=https://github.com/${GITHUB_REPOSITORY}" \
  "${tags[@]}" \
  --file "utilities/docker/Dockerfile.${IMAGE}" \
  .
for prefix in "${prefixes[@]}"; do
  docker push "${prefix}/${IMAGE}:${tag}"
done
