#!/bin/bash
# Deletes the ghcr.io images of this repository that belong neither to main nor to an existing branch.
# Usage: GH_TOKEN=<token> .github/scripts/delete-stale-images.sh [--dry-run]

set -euo pipefail
shopt -s inherit_errexit

dryRun=false
if [ "${1:-}" = --dry-run ]; then
  dryRun=true
elif [ "$#" -gt 0 ]; then
  echo "Usage: $0 [--dry-run]" >&2
  exit 2
fi

owner="${GITHUB_REPOSITORY_OWNER}"
repository="${GITHUB_REPOSITORY}"

declare -A keptTags=([latest]=true)
while read -r branch; do
  keptTags["branch-${branch//[^A-Za-z0-9_.-]/-}"]=true
done < <(gh api --paginate "repos/${repository}/branches" --jq '.[].name')

# Prints the digests that an image index lists.
childDigests() {
  local package="$1" reference="$2" token
  token=$(curl -fsS -u "${GITHUB_ACTOR:-token}:${GH_TOKEN}" \
    "https://ghcr.io/token?scope=repository:${owner,,}/${package}:pull" | jq -r .token)
  curl -fsS -H "Authorization: Bearer ${token}" \
    -H "Accept: application/vnd.oci.image.index.v1+json" \
    -H "Accept: application/vnd.docker.distribution.manifest.list.v2+json" \
    -H "Accept: application/vnd.oci.image.manifest.v1+json" \
    -H "Accept: application/vnd.docker.distribution.manifest.v2+json" \
    "https://ghcr.io/v2/${owner,,}/${package}/manifests/${reference}" | jq -r '.manifests[]?.digest'
}

packages=$(gh api --paginate "orgs/${owner}/packages?package_type=container" \
  --jq ".[] | select(.repository.full_name == \"${repository}\" and .name != \"spack-buildcache\") | .name")

for package in ${packages}; do
  versions=$(gh api --paginate "orgs/${owner}/packages/container/${package}/versions" \
    --jq '.[] | [.id, .name, (.metadata.container.tags | join(","))] | @tsv')
  declare -A keptDigests=()
  untagged=()
  stale=()
  while IFS=$'\t' read -r id digest tags; do
    if [ -z "${id}" ]; then
      continue
    fi
    if [ -z "${tags}" ]; then
      untagged+=("${id} ${digest}")
      continue
    fi
    keep=false
    for tag in ${tags//,/ }; do
      if [ -n "${keptTags[${tag}]:-}" ]; then
        keep=true
      fi
    done
    if [ "${keep}" = true ]; then
      keptDigests["${digest}"]=true
      children=$(childDigests "${package}" "${digest}")
      for child in ${children}; do
        keptDigests["${child}"]=true
      done
    else
      stale+=("${id} ${package}:${tags}")
    fi
  done <<<"${versions}"
  for entry in "${untagged[@]}"; do
    read -r id digest <<<"${entry}"
    if [ -z "${keptDigests[${digest}]:-}" ]; then
      stale+=("${id} ${package}@${digest}")
    fi
  done
  for entry in "${stale[@]}"; do
    read -r id description <<<"${entry}"
    echo "Deleting ${description}"
    if [ "${dryRun}" = false ]; then
      gh api --method DELETE "orgs/${owner}/packages/container/${package}/versions/${id}"
    fi
  done
  unset keptDigests
done
