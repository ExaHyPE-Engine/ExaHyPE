#!/bin/bash
# Prints the source tag of an image. The tag hashes the files of the image in .github/images.txt and the tag of
# its parent at a Git reference.
# Usage: .github/scripts/image-tag.sh <image> [<Git reference, HEAD by default>]

set -euo pipefail

reference="${2:-HEAD}"
root="$(dirname "$0")/../.."
entry=$(git -C "${root}" show "${reference}:.github/images.txt" | grep -E "^$1 " || true)
if [ -z "${entry}" ]; then
  echo "The image $1 is not listed in .github/images.txt of ${reference}" >&2
  exit 2
fi
read -r _ parent files <<<"${entry}"
read -r -a files <<<"${files}"

hash=$({
  if [ "${parent}" != - ]; then
    bash "$0" "${parent}" "${reference}"
  fi
  git -C "${root}" ls-tree -r "${reference}" -- "${files[@]}"
} | sha256sum | cut -c1-16)
echo "sources-${hash}"
