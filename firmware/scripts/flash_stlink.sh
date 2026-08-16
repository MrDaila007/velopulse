#!/usr/bin/env bash
# Flash BikeComp Arduino firmware over ST-Link SWD without erasing the
# Adafruit UF2 bootloader. Also writes bootloader settings (CRC16) so the
# bootloader will start the new application after reset.
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
FIRMWARE_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
ENV_NAME=${1:-xiao_ble_sense}
BUILD_DIR="${FIRMWARE_DIR}/.pio/build/${ENV_NAME}"
HEX="${BUILD_DIR}/firmware.hex"
SETTINGS="${BUILD_DIR}/bootloader_settings.bin"

cd "${FIRMWARE_DIR}"
pio run -e "${ENV_NAME}"

python3 - "${HEX}" "${SETTINGS}" <<'PY'
import struct
import sys
from pathlib import Path

hex_path = Path(sys.argv[1])
settings_path = Path(sys.argv[2])

def parse_hex(path: Path) -> dict[int, int]:
    base = 0
    mem: dict[int, int] = {}
    for line in path.read_text().splitlines():
        if not line.startswith(":"):
            continue
        count = int(line[1:3], 16)
        addr = int(line[3:7], 16)
        rec = int(line[7:9], 16)
        data = bytes.fromhex(line[9 : 9 + 2 * count])
        if rec == 4:
            base = int.from_bytes(data, "big") << 16
        elif rec == 2:
            base = int.from_bytes(data, "big") << 4
        elif rec == 0:
            start = base + addr
            for i, byte in enumerate(data):
                mem[start + i] = byte
    return mem

def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc = (((crc >> 8) & 0xFF) | (crc << 8)) & 0xFFFF
        crc = (crc ^ byte) & 0xFFFF
        crc = (crc ^ ((crc & 0xFF) >> 4)) & 0xFFFF
        crc = (crc ^ ((crc << 8) << 4)) & 0xFFFF
        crc = (crc ^ (((crc & 0xFF) << 4) << 1)) & 0xFFFF
    return crc

mem = parse_hex(hex_path)
if not mem:
    raise SystemExit(f"no data in {hex_path}")
start = min(mem)
end = max(mem) + 1
data = bytes(mem.get(addr, 0xFF) for addr in range(start, end))
print(f"app 0x{start:08X}-0x{end:08X} ({len(data)} bytes) crc16=0x{crc16(data):04X}")
BANK_VALID_APP = 0x01
BANK_ERASED = 0xFE
settings_path.write_bytes(
    struct.pack(
        "<IIIIIIII",
        BANK_VALID_APP,
        crc16(data),
        BANK_ERASED,
        len(data),
        0,
        0,
        0,
        0,
    )
)
PY

openocd -f interface/stlink.cfg -c "transport select hla_swd" -f target/nrf52.cfg \
  -c "adapter speed 1000" \
  -c "init; halt; rbp all" \
  -c "program ${HEX} verify" \
  -c "flash erase_address 0x000FF000 0x1000" \
  -c "flash write_image ${SETTINGS} 0x000FF000" \
  -c "verify_image ${SETTINGS} 0x000FF000" \
  -c "mww 0x20007F7C 0" \
  -c "mww 0x4000051C 0" \
  -c "mww 0xE000ED0C 0x05FA0004" \
  -c "shutdown"

echo "Flashed ${HEX} over ST-Link. Device should advertise BikeComp-XXXX."
