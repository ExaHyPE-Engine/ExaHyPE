#!/bin/bash
# Prints whether the engine tests run, which test families they run (full or branch), whether they build the
# images of the branch, the branch and the image tag.
# Usage: GH_TOKEN=<token> .github/scripts/plan-engine-tests.sh

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
images=true
branch="${GITHUB_REF_NAME}"
case "${GITHUB_EVENT_NAME}" in
  push)
    if [ "${branch}" != main ]; then
      scope=branch
      # The pull request of a branch of this repository runs the full engine tests of the same commit.
      openRequests=$(gh pr list --repo "${GITHUB_REPOSITORY}" --head "${branch}" --state open \
        --json headRepositoryOwner --jq "map(select(.headRepositoryOwner.login == \"${GITHUB_REPOSITORY_OWNER}\")) | length")
      if [ "${openRequests}" -gt 0 ]; then
        run=false
      fi
    fi
    ;;
  pull_request)
    branch="${HEAD_BRANCH}"
    if [ "${HEAD_REPOSITORY}" != "${GITHUB_REPOSITORY}" ]; then
      images=false
      branch=main
    fi
    ;;
esac

echo "run=${run}"
echo "scope=${scope}"
echo "images=${images}"
echo "branch=${branch}"
echo "tag=$(tagOf "${branch}")"
