#!/bin/bash
# Waits up to five hours for the source tag of every image on ghcr.io and writes <image>=<reference> to
# GITHUB_OUTPUT. With --no-wait, a missing tag fails at once.
# Usage: GHCR_TOKEN=<token> .github/scripts/wait-for-images.sh [--no-wait] <image>...

set -euo pipefail

wait=true
if [ "$1" = --no-wait ]; then
  wait=false
  shift
fi
registry="ghcr.io/${GITHUB_REPOSITORY_OWNER,,}"

docker login ghcr.io --username "${GITHUB_ACTOR}" --password-stdin <<<"${GHCR_TOKEN}" >/dev/null

for image in "$@"; do
  reference="${registry}/${image}:$(bash "$(dirname "$0")/image-tag.sh" "${image}")"
  minutes=0
  until docker buildx imagetools inspect "${reference}" >/dev/null 2>&1; do
    if [ "${wait}" = false ] || [ "${minutes}" -ge 300 ]; then
      echo "::error::${reference} was not built. The workflow Images builds it on a push to this repository."
      exit 1
    fi
    echo "Waiting for ${reference}"
    sleep 60
    minutes=$((minutes + 1))
  done
  echo "${image}=${reference}" | tee -a "${GITHUB_OUTPUT}"
done
