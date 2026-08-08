#!/usr/bin/env python3
"""Serial-side checks for OLED 128x64 hardware gate."""

from __future__ import annotations

import argparse
import re
import sys
import time

from serial_client import open_port, send_command

SELFTEST_RE = re.compile(r"selftest=0x([0-9A-Fa-f]+)")
DISPLAY_STATE_RE = re.compile(r"Display: power=(bright|dim|off)")
DISPLAY_TIMEOUT_RE = re.compile(r"display_timeout_s=(\d+)")


def read_display_state(port) -> str | None:
    output = send_command(port, "display-state", wait_s=0.8)
    match = DISPLAY_STATE_RE.search(output)
    return match.group(1) if match else None


def main() -> int:
    parser = argparse.ArgumentParser(description="OLED serial gate checks")
    parser.add_argument("--port", default="/dev/ttyACM1")
    parser.add_argument("--skip-timing", action="store_true")
    args = parser.parse_args()

    failures: list[str] = []
    notes: list[str] = []

    with open_port(args.port) as port:
        config_out = send_command(port, "dump-config", wait_s=2.0)
        selftest_out = send_command(port, "selftest", wait_s=2.0)

        timeout_match = DISPLAY_TIMEOUT_RE.search(config_out)
        display_timeout_s = int(timeout_match.group(1)) if timeout_match else 60
        notes.append(f"display_timeout_s={display_timeout_s}")

        selftest_match = SELFTEST_RE.search(selftest_out)
        if not selftest_match:
            failures.append("selftest mask missing from Serial output")
        else:
            mask = int(selftest_match.group(1), 16)
            if mask != 0x3F:
                failures.append(f"selftest mask 0x{mask:02X} != 0x3F")
        if "i2c_err=0" not in selftest_out:
            failures.append("i2c_err != 0")
        if "isr_ovf=0" not in selftest_out:
            failures.append("isr_ovf != 0")
        if "valid=0" in selftest_out and "raw=0" in selftest_out:
            notes.append("LDR absent: invalid fallback confirmed")

        if not args.skip_timing and not failures:
            dim_s = max(5, display_timeout_s // 2)
            off_s = max(10, display_timeout_s)
            notes.append(f"waiting {dim_s}s for dim")
            time.sleep(dim_s + 1)
            state = read_display_state(port)
            if state != "dim":
                failures.append(f"expected dim after {dim_s}s, got {state!r}")
            else:
                notes.append("dim state confirmed")

            remaining = max(1, off_s - dim_s)
            notes.append(f"waiting {remaining}s for off")
            time.sleep(remaining + 1)
            state = read_display_state(port)
            if state != "off":
                failures.append(f"expected off after {off_s}s, got {state!r}")
            else:
                notes.append("off state confirmed")

            send_command(port, "wake-display", wait_s=0.5)
            state = read_display_state(port)
            if state != "bright":
                failures.append(f"expected bright after wake-display, got {state!r}")
            else:
                notes.append("wake via wake-display confirmed")

    print("OLED serial gate")
    for note in notes:
        print(f"  note: {note}")
    if failures:
        print("FAIL:")
        for item in failures:
            print(f"  - {item}")
        return 1

    print("PASS: serial selftest, config, dim/off/wake checks")
    print("Manual: LOW BATT visual, DISPLAY_TEST patterns, 4x60s burn-in shift")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
