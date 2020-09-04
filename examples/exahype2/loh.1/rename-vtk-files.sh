#!/usr/bin/env bash
for f in *.vtu; do
  snapshot=$(echo "$f" | grep "\-[0-9]\+\-" -o | grep "[0-9]\+" -o )
  fOut=${f/-$snapshot/}
  echo "mv $f ${fOut/.vtu/-${snapshot}.vtu}"
  mv $f ${fOut/.vtu/-${snapshot}.vtu}
done
