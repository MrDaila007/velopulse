#!/usr/bin/env bash
# Serial console for BikeComp Zephyr firmware (115200, same as PIO monitor_speed).
set -euo pipefail

BAUD="${MONITOR_SPEED:-115200}"
PORT="${UPLOAD_PORT:-${MONITOR_PORT:-}}"

usage() {
  cat <<EOF
Usage: $(basename "$0") [-p PORT] [-b BAUD]

Open USB CDC console. Set UPLOAD_PORT or pass -p (e.g. /dev/ttyACM0).

Environment:
  UPLOAD_PORT / MONITOR_PORT   Serial device
  MONITOR_SPEED                Baud rate (default: 115200)
EOF
}

while getopts "p:b:h" opt; do
  case "${opt}" in
    p) PORT="${OPTARG}" ;;
    b) BAUD="${OPTARG}" ;;
    h) usage; exit 0 ;;
    *) usage; exit 1 ;;
  esac
done

if [[ -z "${PORT}" ]]; then
  if command -v pio >/dev/null 2>&1; then
    PORT="$(pio device list --serial --json-output 2>/dev/null | python3 -c "
import json, sys
data = json.load(sys.stdin)
vids = {0x2886, 0x239A}
for entry in data:
    info = entry.get('port') or entry
    vid = info.get('hwid_vid')
    if vid is None:
        continue
    try:
        if int(vid, 16) in vids:
            print(info.get('port') or info.get('device', ''))
            break
    except ValueError:
        pass
" 2>/dev/null || true)"
  fi
fi

if [[ -z "${PORT}" ]]; then
  echo "Set UPLOAD_PORT or pass -p PORT." >&2
  exit 1
fi

if python3 -c "import serial.tools.miniterm" 2>/dev/null; then
  exec python3 -m serial.tools.miniterm "${PORT}" "${BAUD}" --raw
fi

if command -v pio >/dev/null 2>&1; then
  exec pio device monitor -p "${PORT}" -b "${BAUD}"
fi

echo "Install pyserial or PlatformIO for serial monitor." >&2
exit 1
