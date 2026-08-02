#!/usr/bin/env bash
# Run BikeComp Zephyr ztest domain suite (host unittest via Twister).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
TEST_DIR="${ROOT}/tests/domain"

# shellcheck source=ensure_west_zephyr.sh
source "${ROOT}/scripts/ensure_west_zephyr.sh"

if [ -z "${ZEPHYR_BASE:-}" ]; then
  if [ -d "${REPO_ROOT}/zephyr" ]; then
    export ZEPHYR_BASE="${REPO_ROOT}/zephyr"
  elif [ -d "/data/zephyrproject-v4.4/zephyr" ]; then
    export ZEPHYR_BASE="/data/zephyrproject-v4.4/zephyr"
  elif [ -d "${REPO_ROOT}/deps/zephyr" ]; then
    export ZEPHYR_BASE="${REPO_ROOT}/deps/zephyr"
  else
    echo "Set ZEPHYR_BASE or run ${ROOT}/scripts/bootstrap.sh" >&2
    exit 1
  fi
fi

if [ ! -f "${ZEPHYR_BASE}/scripts/twister" ]; then
  echo "Twister not found under ZEPHYR_BASE=${ZEPHYR_BASE}" >&2
  exit 1
fi

if [ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ]; then
  for candidate in \
    "${REPO_ROOT}/zephyr-sdk-1.0.0" \
    /data/zephyr-sdk-1.0.0; do
    if [ -d "${candidate}" ]; then
      export ZEPHYR_SDK_INSTALL_DIR="${candidate}"
      break
    fi
  done
fi

if [ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ] || [ ! -f "${ZEPHYR_SDK_INSTALL_DIR}/cmake/Zephyr-sdkConfig.cmake" ]; then
  echo "Zephyr SDK not found. Set ZEPHYR_SDK_INSTALL_DIR (required by Zephyr 4.4 twister)." >&2
  exit 1
fi

hide_west_if_incomplete "${REPO_ROOT}"

cd "${REPO_ROOT}"
set +e
python3 "${ZEPHYR_BASE}/scripts/twister" -T "${TEST_DIR}" --ninja -v "$@"
status=$?
set -e
restore_west_if_hidden "${REPO_ROOT}"
exit "${status}"
