#!/bin/bash
# Deletes every image version on ghcr.io that is neither latest nor the source tag of a branch or an open pull
# request.
# Usage: GH_TOKEN=<token> .github/scripts/delete-stale-images.sh [--dry-run]

set -euo pipefail

dryRun=false
if [ "${1:-}" = --dry-run ]; then
  dryRun=true
fi
owner="${GITHUB_REPOSITORY_OWNER}"
images=$(grep -vE '^#|^$' .github/images.txt | awk '{print $1}')

git fetch --quiet origin '+refs/heads/*:refs/remotes/origin/*' '+refs/pull/*/merge:refs/remotes/pull/*'

declare -A used=()
for reference in $(git for-each-ref --format='%(refname)' refs/remotes/origin refs/remotes/pull); do
  for image in ${images}; do
    if tag=$(bash .github/scripts/image-tag.sh "${image}" "${reference}" 2>/dev/null); then
      used["${image}:${tag}"]=true
    fi
  done
done

for image in ${images}; do
  versions=$(gh api --paginate "orgs/${owner}/packages/container/${image}/versions" \
    --jq '.[] | [.id, (.metadata.container.tags | join(","))] | @tsv' 2>/dev/null || true)
  while IFS=$'\t' read -r id tags; do
    if [ -z "${id}" ]; then
      continue
    fi
    keep=false
    for tag in ${tags//,/ }; do
      if [ "${tag}" = latest ] || [ -n "${used[${image}:${tag}]:-}" ]; then
        keep=true
      fi
    done
    if [ "${keep}" = false ]; then
      echo "Deleting ${image} version ${id} (tags: ${tags:-none})"
      if [ "${dryRun}" = false ]; then
        gh api --method DELETE "orgs/${owner}/packages/container/${image}/versions/${id}"
      fi
    fi
  done <<<"${versions}"
done
