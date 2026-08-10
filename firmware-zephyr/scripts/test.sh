#!/usr/bin/env bash
# Run BikeComp Zephyr ztest domain suite (host unittest via Twister).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
TEST_DIR="${ROOT}/tests/domain"

# shellcheck source=env.sh
source "${ROOT}/scripts/env.sh"
bikecomp_load_env

# shellcheck source=ensure_west_zephyr.sh
source "${ROOT}/scripts/ensure_west_zephyr.sh"

if [ -z "${ZEPHYR_BASE:-}" ]; then
  echo "Zephyr not found. Run ${ROOT}/scripts/configure-env.sh or bootstrap.sh." >&2
  exit 1
fi

if [ ! -f "${ZEPHYR_BASE}/scripts/twister" ]; then
  echo "Twister not found under ZEPHYR_BASE=${ZEPHYR_BASE}" >&2
  exit 1
fi

if [ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ] || [ ! -f "${ZEPHYR_SDK_INSTALL_DIR}/cmake/Zephyr-sdkConfig.cmake" ]; then
  echo "Zephyr SDK not found. Set ZEPHYR_SDK_INSTALL_DIR (required by Zephyr 4.4 twister)." >&2
  exit 1
fi

hide_west_if_incomplete "${REPO_ROOT}"

cd "${REPO_ROOT}"
mkdir -p "${USER_CACHE_DIR}" "${CCACHE_DIR}" "${CCACHE_TEMPDIR}"
set +e
"${ZEPHYR_PYTHON}" "${ZEPHYR_BASE}/scripts/twister" -T "${TEST_DIR}" --ninja -v "$@"
status=$?
set -e
restore_west_if_hidden "${REPO_ROOT}"
exit "${status}"
