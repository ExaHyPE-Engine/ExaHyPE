#!/bin/bash
# Prints whether the CI runs, which test families it runs (full or branch) and the tag of its images.
# Usage: GH_TOKEN=<token> .github/scripts/plan-ci.sh

set -euo pipefail

# Prints the image tag of a branch.
tagOf() {
  if [ "$1" = main ]; then
    echo latest
  else
    echo "branch-${1//[^A-Za-z0-9_.-]/-}"
  fi
}

run=true
scope=full
tag=latest
case "${GITHUB_EVENT_NAME}" in
  push)
    branch="${GITHUB_REF_NAME}"
    tag=$(tagOf "${branch}")
    if [ "${branch}" != main ]; then
      scope=branch
      # The pull request of a branch of this repository runs the full CI of the same commit.
      openRequests=$(gh pr list --repo "${GITHUB_REPOSITORY}" --head "${branch}" --state open \
        --json headRepositoryOwner --jq "map(select(.headRepositoryOwner.login == \"${GITHUB_REPOSITORY_OWNER}\")) | length")
      if [ "${openRequests}" -gt 0 ]; then
        run=false
      fi
    fi
    ;;
  pull_request)
    if [ "${HEAD_REPOSITORY}" = "${GITHUB_REPOSITORY}" ]; then
      tag=$(tagOf "${HEAD_BRANCH}")
    fi
    ;;
  *)
    tag=$(tagOf "${GITHUB_REF_NAME}")
    ;;
esac

echo "run=${run}"
echo "scope=${scope}"
echo "tag=${tag}"
