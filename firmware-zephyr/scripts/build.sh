#!/usr/bin/env bash
# Build BikeComp Zephyr firmware for XIAO nRF52840 Sense (Super-nRF52840 overlay).
set -euo pipefail

export PATH="${HOME}/.local/bin:${PATH}"

# Prefer west from a local Zephyr workspace venv.
if [ -x "/data/zephyrproject-v4.4/.venv/bin/west" ]; then
  export PATH="/data/zephyrproject-v4.4/.venv/bin:${PATH}"
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"

BOARD="${BOARD:-xiao_ble/nrf52840}"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
OVERLAY="${OVERLAY:-${ROOT}/app/boards/super_nrf52840.overlay}"

# Zephyr SDK: env override, then common install locations.
if [ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ]; then
  for candidate in \
    /data/zephyr-sdk-1.0.0 \
    "${HOME}/zephyr-sdk-0.16.8" \
    "${HOME}/zephyr-sdk-0.17.0"; do
    if [ -d "${candidate}" ]; then
      export ZEPHYR_SDK_INSTALL_DIR="${candidate}"
      break
    fi
  done
fi

if [ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ] || [ ! -d "${ZEPHYR_SDK_INSTALL_DIR}" ]; then
  echo "Set ZEPHYR_SDK_INSTALL_DIR to a Zephyr SDK install directory." >&2
  exit 1
fi

# Zephyr tree: env override, repo deps/, or shared workspace on disk.
if [ -n "${ZEPHYR_BASE:-}" ] && [ -d "${ZEPHYR_BASE}" ]; then
  :
elif [ -d "${REPO_ROOT}/deps/zephyr" ]; then
  export ZEPHYR_BASE="${REPO_ROOT}/deps/zephyr"
elif [ -d "/data/zephyrproject-v4.4/zephyr" ]; then
  export ZEPHYR_BASE="/data/zephyrproject-v4.4/zephyr"
else
  echo "Zephyr not found. Set ZEPHYR_BASE or run ${ROOT}/scripts/bootstrap.sh" >&2
  exit 1
fi

if [ -f "${ZEPHYR_BASE}/zephyr-env.sh" ]; then
  # shellcheck disable=SC1090
  source "${ZEPHYR_BASE}/zephyr-env.sh"
fi

# Prefer an existing full west workspace (modules on CMAKE_PREFIX_PATH).
WEST_TOP="${WEST_TOP:-}"
if [ -z "${WEST_TOP}" ]; then
  if [ -d "/data/zephyrproject-v4.4/.west" ]; then
    WEST_TOP="/data/zephyrproject-v4.4"
  else
    WEST_TOP="${REPO_ROOT}"
  fi
fi

cd "${WEST_TOP}"
west build -d "${BUILD_DIR}" -b "${BOARD}" "${ROOT}/app" \
  -- \
  -DEXTRA_DTC_OVERLAY_FILE="${OVERLAY}"

echo "Artifacts: ${BUILD_DIR}/zephyr/zephyr.{hex,uf2}"
