#!/bin/bash
# Builds utilities/docker/Dockerfile.<image> and pushes it to ghcr.io under its source tag if that tag is missing.
# On main, it also tags the image latest.
# Usage: GHCR_TOKEN=<token> .github/scripts/build-image.sh <image>

set -euo pipefail

image="$1"
scripts="$(dirname "$0")"
registry="ghcr.io/${GITHUB_REPOSITORY_OWNER,,}"
reference="${registry}/${image}:$(bash "${scripts}/image-tag.sh" "${image}")"

docker login ghcr.io --username "${GITHUB_ACTOR}" --password-stdin <<<"${GHCR_TOKEN}"

if docker buildx imagetools inspect "${reference}" >/dev/null 2>&1; then
  echo "${reference} exists"
else
  bash "${scripts}/prepare-runner.sh"
  docker build \
    --build-arg "IMAGE_PREFIX=${registry}" \
    --build-arg "BASE_TAG=$(bash "${scripts}/image-tag.sh" base)" \
    --build-arg "SPACK_TAG=$(bash "${scripts}/image-tag.sh" spack)" \
    --build-arg "EXAHYPE_TAG=$(bash "${scripts}/image-tag.sh" exahype)" \
    --build-arg "SPACK_BUILDCACHE=oci://${registry}/spack-buildcache" \
    --secret id=spack_buildcache_user,env=GITHUB_ACTOR \
    --secret id=spack_buildcache_token,env=GHCR_TOKEN \
    --label "org.opencontainers.image.source=https://github.com/${GITHUB_REPOSITORY}" \
    --tag "${reference}" \
    --file "utilities/docker/Dockerfile.${image}" \
    .
  docker push "${reference}"
fi

if [ "${GITHUB_REF_NAME}" = main ]; then
  docker buildx imagetools create --tag "${registry}/${image}:latest" "${reference}"
fi
