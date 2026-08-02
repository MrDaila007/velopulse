#!/usr/bin/env bash
# Build BikeComp Zephyr firmware for XIAO nRF52840 Sense (Super-nRF52840 overlay).
set -euo pipefail

export PATH="${HOME}/.local/bin:${PATH}"

if [ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ] && [ -d "${HOME}/zephyr-sdk-0.16.8" ]; then
  export ZEPHYR_SDK_INSTALL_DIR="${HOME}/zephyr-sdk-0.16.8"
fi

if [ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ]; then
  echo "Set ZEPHYR_SDK_INSTALL_DIR to Zephyr SDK 0.16.x before building." >&2
  echo "See https://github.com/zephyrproject-rtos/sdk-ng/releases" >&2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"

BOARD="${BOARD:-xiao_ble/nrf52840}"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
OVERLAY="${OVERLAY:-${ROOT}/app/boards/super_nrf52840.overlay}"

if [ ! -d "${REPO_ROOT}/deps/zephyr" ]; then
  echo "Zephyr not bootstrapped. Run ${ROOT}/scripts/bootstrap.sh first." >&2
  exit 1
fi

export ZEPHYR_BASE="${REPO_ROOT}/deps/zephyr"
if [ -f "${ZEPHYR_BASE}/zephyr-env.sh" ]; then
  # shellcheck disable=SC1090
  source "${ZEPHYR_BASE}/zephyr-env.sh"
fi

cd "${REPO_ROOT}"
python3 -m west build -d "${BUILD_DIR}" -b "${BOARD}" "${ROOT}/app" \
  -- \
  -DEXTRA_DTC_OVERLAY_FILE="${OVERLAY}"

echo "Artifacts: ${BUILD_DIR}/zephyr/zephyr.{hex,uf2}"
