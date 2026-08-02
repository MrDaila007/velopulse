#!/usr/bin/env bash
# Bootstrap Zephyr west workspace for BikeComp firmware-zephyr.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"

if ! command -v west >/dev/null 2>&1; then
  python3 -m pip install --user -U west
  export PATH="${HOME}/.local/bin:${PATH}"
fi

if [ ! -d "${REPO_ROOT}/.west" ]; then
  cd "${REPO_ROOT}"
  west init -l "${ROOT}"
fi

cd "${REPO_ROOT}"
west update
west zephyr-export

echo "Zephyr workspace ready at ${REPO_ROOT} (ZEPHYR_BASE=${REPO_ROOT}/deps/zephyr)"
echo "Build with: ${ROOT}/scripts/build.sh"
