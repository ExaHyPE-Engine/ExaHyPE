#!/bin/bash
# Runs every style and import check and reports each failing check.
# Usage: .github/scripts/check-style.sh

set -euo pipefail

Checks=(
  "apply_clang_format --check" "apply_black --check" check_includes check_imports check_python_lint check_dead_code
)
failures=()

for check in "${Checks[@]}"; do
  echo "::group::${check}"
  read -r -a command <<<"${check}"
  if ! python3 -m "postprocessing.${command[0]}" "${command[@]:1}"; then
    echo "::error::${check} failed"
    failures+=("${check}")
  fi
  echo "::endgroup::"
done

if [ "${#failures[@]}" -gt 0 ]; then
  printf 'Failed: %s\n' "${failures[@]}"
  exit 1
fi
