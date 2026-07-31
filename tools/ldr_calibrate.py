#!/usr/bin/env python3
"""Collect LDR raw samples over Serial and suggest calibration constants."""

from __future__ import annotations

import argparse
import re
import sys
import time

from serial_client import open_port, send_command

AMBIENT_RE = re.compile(
    r"Ambient: enabled=\d, valid=(\d), raw=(\d+), filtered=(\d+), "
    r"auto_pct=(\d+), effective_pct=(\d+)"
)


def parse_ambient_lines(text: str) -> list[dict[str, int]]:
    rows: list[dict[str, int]] = []
    for match in AMBIENT_RE.finditer(text):
        rows.append(
            {
                "valid": int(match.group(1)),
                "raw": int(match.group(2)),
                "filtered": int(match.group(3)),
                "auto_pct": int(match.group(4)),
                "effective_pct": int(match.group(5)),
            }
        )
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description="LDR calibration sampler")
    parser.add_argument("--port", default="/dev/ttyACM1")
    parser.add_argument("--seconds", type=float, default=15.0)
    parser.add_argument("--interval", type=float, default=1.0)
    args = parser.parse_args()

    with open_port(args.port) as port:
        send_command(port, "ambient-raw", wait_s=0.5)
        deadline = time.time() + args.seconds
        chunks: list[str] = []
        while time.time() < deadline:
            time.sleep(args.interval)
            chunks.append(port.read(4096).decode("utf-8", errors="replace"))
        send_command(port, "ambient-stop", wait_s=0.3)

    text = "".join(chunks)
    rows = parse_ambient_lines(text)
    if not rows:
        print("No ambient samples received.", file=sys.stderr)
        print(text, file=sys.stderr)
        return 1

    valid_rows = [row for row in rows if row["valid"] == 1]
    if not valid_rows:
        print("LDR not detected (valid=0 for all samples).")
        print("Assemble divider on D2/D3 per hardware/wiring.md and retry.")
        print(f"Last sample: raw={rows[-1]['raw']}, valid={rows[-1]['valid']}")
        return 2

    raw_values = [row["raw"] for row in valid_rows]
    raw_dark = min(raw_values)
    raw_bright = max(raw_values)
    margin = max(20, int((raw_bright - raw_dark) * 0.05))
    suggested_dark = max(0, raw_dark - margin)
    suggested_bright = min(4095, raw_bright + margin)

    print(f"Samples: {len(valid_rows)} valid / {len(rows)} total")
    print(f"Observed raw range: {raw_dark} .. {raw_bright}")
    print("Suggested platformio.ini constants:")
    print(f"  -DBIKECOMP_AMBIENT_RAW_DARK={suggested_dark}")
    print(f"  -DBIKECOMP_AMBIENT_RAW_BRIGHT={suggested_bright}")
    if raw_bright - raw_dark < 500:
        print("WARNING: narrow ADC span; try R_FIXED 10 kΩ or 47 kΩ.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
