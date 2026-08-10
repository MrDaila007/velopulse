#!/usr/bin/env bash
# Build BikeComp Zephyr firmware for XIAO nRF52840 Sense (Super-nRF52840 overlay).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# shellcheck source=env.sh
source "${ROOT}/scripts/env.sh"
bikecomp_load_env
bikecomp_require_build_env

# shellcheck source=ensure_west_zephyr.sh
source "${ROOT}/scripts/ensure_west_zephyr.sh"

BOARD="${BOARD:-xiao_ble/nrf52840}"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
OVERLAY="${OVERLAY:-${ROOT}/app/boards/super_nrf52840.overlay}"

if [ -f "${ZEPHYR_BASE}/zephyr-env.sh" ]; then
  # shellcheck disable=SC1090
  source "${ZEPHYR_BASE}/zephyr-env.sh"
fi

mkdir -p "${USER_CACHE_DIR}" "${CCACHE_DIR}" "${CCACHE_TEMPDIR}"

cd "${WEST_TOP}"
CONF_ARGS=(-DEXTRA_DTC_OVERLAY_FILE="${OVERLAY}")
CONF_ARGS+=(-DUSER_CACHE_DIR="${USER_CACHE_DIR}")
if grep -q '^CONFIG_BIKECOMP_ZEPHYR_BLE_STUB=n' "${ROOT}/app/prj.conf" 2>/dev/null; then
  CONF_ARGS+=(-DEXTRA_CONF_FILE="${ROOT}/app/conf/overlay-bt.conf")
fi
"${WEST_BIN}" build -d "${BUILD_DIR}" -b "${BOARD}" "${ROOT}/app" -- "${CONF_ARGS[@]}"

echo "Artifacts: ${BUILD_DIR}/zephyr/zephyr.{hex,uf2}"
