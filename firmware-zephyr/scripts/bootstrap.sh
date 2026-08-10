#!/usr/bin/env bash
# Bootstrap Zephyr west workspace for BikeComp firmware-zephyr.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"

# shellcheck source=env.sh
source "${ROOT}/scripts/env.sh"
bikecomp_load_env

if [[ -z "${WEST_BIN:-}" || -z "${ZEPHYR_PYTHON:-}" ]] ||
  ! "${ZEPHYR_PYTHON:-/bin/false}" -c \
    'import sys; raise SystemExit(sys.version_info < (3, 12))'; then
  BOOTSTRAP_PYTHON="$(command -v python3.12 || command -v python3 || true)"
  if [[ -z "${BOOTSTRAP_PYTHON}" ]] ||
    ! "${BOOTSTRAP_PYTHON}" -c 'import sys; raise SystemExit(sys.version_info < (3, 12))'; then
    echo "Zephyr 4.4 requires Python 3.12 or newer." >&2
    exit 1
  fi
  "${BOOTSTRAP_PYTHON}" -m venv "${REPO_ROOT}/.venv"
  "${REPO_ROOT}/.venv/bin/python" -m pip install -U pip west
  WEST_BIN="${REPO_ROOT}/.venv/bin/west"
  ZEPHYR_PYTHON="${REPO_ROOT}/.venv/bin/python"
  export WEST_BIN
  export ZEPHYR_PYTHON
fi

if [ ! -d "${REPO_ROOT}/.west" ]; then
  cd "${REPO_ROOT}"
  "${WEST_BIN}" init -l "${ROOT}"
fi

cd "${REPO_ROOT}"
"${WEST_BIN}" update
"${WEST_BIN}" zephyr-export

ZEPHYR_BASE="${REPO_ROOT}/deps/zephyr" \
WEST_TOP="${REPO_ROOT}" \
WEST_BIN="${WEST_BIN}" \
U8G2_ROOT="${REPO_ROOT}/deps/u8g2" \
  "${ROOT}/scripts/configure-env.sh"

echo "Zephyr workspace ready at ${REPO_ROOT} (ZEPHYR_BASE=${REPO_ROOT}/deps/zephyr)"
echo "Build with: ${ROOT}/scripts/build.sh"
