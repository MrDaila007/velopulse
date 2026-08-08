#!/usr/bin/env bash
# Flash BikeComp Zephyr firmware via Adafruit nRF52 serial DFU (PIO upload_protocol=nrfutil).
set -euo pipefail

export PATH="${HOME}/.local/bin:/data/zephyrproject-v4.4/.venv/bin:${PATH}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
HEX="${BUILD_DIR}/zephyr/zephyr.hex"

NO_BUILD=0
UPLOAD_ARGS=()

usage() {
  cat <<EOF
Usage: $(basename "$0") [options] [-- upload.py args]

Build (unless --no-build) and upload over USB serial using adafruit-nrfutil.

Options:
  --no-build       Skip west build; require existing ${HEX}
  -p, --port PORT  Serial port (default: auto / UPLOAD_PORT)
  --no-touch       Board already in bootloader (double-tap RESET)
  -h, --help       Show this help

Environment:
  UPLOAD_PORT           Default serial port
  ADAFRUIT_NRFUTIL      Path to adafruit-nrfutil.py (PIO package is auto-detected)
  BUILD_DIR             Build output directory

Examples:
  ./scripts/upload.sh
  UPLOAD_PORT=/dev/ttyACM0 ./scripts/upload.sh --no-build
  ./scripts/upload.sh --no-touch -p /dev/ttyACM0
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-build)
      NO_BUILD=1
      shift
      ;;
    -p|--port)
      UPLOAD_ARGS+=("-p" "$2")
      shift 2
      ;;
    --no-touch)
      UPLOAD_ARGS+=("--no-touch")
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      UPLOAD_ARGS+=("$@")
      break
      ;;
    *)
      UPLOAD_ARGS+=("$1")
      shift
      ;;
  esac
done

if [[ "${NO_BUILD}" -eq 0 ]]; then
  "${ROOT}/scripts/build.sh"
fi

if [[ ! -f "${HEX}" ]]; then
  echo "Missing ${HEX}" >&2
  exit 1
fi

exec python3 "${ROOT}/scripts/serial_upload.py" --hex "${HEX}" "${UPLOAD_ARGS[@]}"
