#!/bin/bash
# Frees disk on a GitHub-hosted runner and moves the Docker storage to its larger second disk /mnt.
# Usage: .github/scripts/prepare-runner.sh

set -euo pipefail

sudo rm -rf /usr/local/lib/android /usr/share/dotnet /opt/ghc /opt/hostedtoolcache /usr/local/.ghcup \
  /usr/share/swift /usr/lib/jvm /usr/local/share/boost /opt/microsoft /opt/google /usr/local/share/chromium
sudo docker image prune --all --force >/dev/null

rootFree=$(df --output=avail -B1 / | tail -1)
mntFree=$(df --output=avail -B1 /mnt | tail -1)
if [ "${mntFree}" -gt "${rootFree}" ]; then
  configuration=/etc/docker/daemon.json
  if [ -s "${configuration}" ]; then
    jq '. + {"data-root": "/mnt/docker"}' "${configuration}" >/tmp/daemon.json
  else
    echo '{"data-root": "/mnt/docker"}' >/tmp/daemon.json
  fi
  sudo systemctl stop docker.socket docker
  sudo install -m 644 /tmp/daemon.json "${configuration}"
  sudo systemctl start docker
fi

df -h / /mnt
docker info --format 'Docker storage: {{.DockerRootDir}}'
