#!/usr/bin/env bash
# Run BikeComp Zephyr ztest domain suite (host unittest via Twister).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
TEST_DIR="${ROOT}/tests/domain"

if [ -z "${ZEPHYR_BASE:-}" ]; then
  if [ -d "/data/zephyrproject-v4.4/zephyr" ]; then
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

cd "${REPO_ROOT}"
exec python3 "${ZEPHYR_BASE}/scripts/twister" -T "${TEST_DIR}" --ninja -v "$@"
