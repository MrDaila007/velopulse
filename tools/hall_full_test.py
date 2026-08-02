#!/usr/bin/env python3
"""Full Hall/reed diagnostic over BikeComp Serial (D0+D1 digital)."""

from __future__ import annotations

import argparse
import glob
import re
import sys
import time

from serial_client import open_port, send_command

GPIO_DIGITAL_RE = re.compile(
    r"GPIO D0\+D1.*: PULLUP=(HIGH|LOW), FLOAT=(HIGH|LOW), PULLDOWN=(HIGH|LOW)"
)
HALL_RE = re.compile(
    r"Hall: pin=D0\+D1.*level=(HIGH|LOW), .*edge=(\w+), raw_pulses=(\d+), "
    r"accepted=(\d+)"
)


def find_port(explicit: str | None) -> str:
    if explicit:
        return explicit
    by_id = sorted(glob.glob("/dev/serial/by-id/*"))
    for path in by_id:
        lower = path.lower()
        if any(token in lower for token in ("seeed", "xiao", "nrf", "52840")):
            return path
    for path in ("/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyUSB0"):
        if glob.glob(path):
            return path
    raise SystemExit(
        "Serial port not found. Plug in Super-nRF52840 via USB and retry, "
        "or pass --port /dev/ttyACM0"
    )


def parse_gpio_digital(text: str) -> tuple[str, str, str] | None:
    match = GPIO_DIGITAL_RE.search(text)
    if not match:
        return None
    return match.group(1), match.group(2), match.group(3)


def parse_hall(text: str) -> dict[str, int | str] | None:
    match = HALL_RE.search(text)
    if not match:
        return None
    return {
        "level": match.group(1),
        "edge": match.group(2),
        "raw_pulses": int(match.group(3)),
        "accepted": int(match.group(4)),
    }


def collect_hall_watch(port, seconds: float) -> list[dict[str, int | str]]:
    port.reset_input_buffer()
    port.write(b"hall-watch\n")
    time.sleep(0.3)
    rows: list[dict[str, int | str]] = []
    deadline = time.time() + seconds
    while time.time() < deadline:
        chunk = port.read(4096).decode("utf-8", errors="replace")
        for line in chunk.splitlines():
            parsed = parse_hall(line)
            if parsed:
                rows.append(parsed)
        time.sleep(0.1)
    port.write(b"hall-stop\n")
    time.sleep(0.2)
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description="Run full Hall/reed diagnostics")
    parser.add_argument("--port", default=None)
    parser.add_argument("--watch-seconds", type=float, default=20.0)
    args = parser.parse_args()

    port_path = find_port(args.port)
    print(f"Using serial port: {port_path}")
    print("Wiring expected: D0 <-> reed <-> D1 (D0 drive LOW, D1 sense)")
    print()

    with open_port(port_path) as port:
        time.sleep(0.4)
        boot = port.read(4096).decode("utf-8", errors="replace")
        if boot.strip():
            print("Boot banner:")
            print(boot.strip())
            print()

        config_out = send_command(port, "dump-config", wait_s=1.2)
        edge_match = re.search(r"active_edge=(\d+)", config_out)
        debounce_match = re.search(r"debounce_ms=(\d+)", config_out)
        print(
            "Config:",
            f"active_edge={edge_match.group(1) if edge_match else '?'}",
            f"debounce_ms={debounce_match.group(1) if debounce_match else '?'}",
        )

        probe_out = send_command(port, "gpio-probe", wait_s=0.8)
        print(probe_out.strip())
        baseline = parse_gpio_digital(probe_out)
        if not baseline:
            print("FAIL: could not parse gpio-probe (need D0+D1 digital firmware)", file=sys.stderr)
            return 1
        pullup, float_level, pulldown = baseline
        print(f"Baseline: PULLUP={pullup}, FLOAT={float_level}, PULLDOWN={pulldown}")
        print()

        print(f"Hall watch {args.watch_seconds:.0f}s — move magnet near/away from reed...")
        watch_rows = collect_hall_watch(port, args.watch_seconds)
        levels = sorted({str(r["level"]) for r in watch_rows})
        if watch_rows:
            start = watch_rows[0]
            end = watch_rows[-1]
            raw_delta = int(end["raw_pulses"]) - int(start["raw_pulses"])
            print(
                f"  raw_pulses {start['raw_pulses']} -> {end['raw_pulses']} "
                f"(delta {raw_delta}), accepted={end['accepted']}, levels={levels}"
            )
        else:
            raw_delta = 0
            print("  no hall-watch lines")

        status_out = send_command(port, "hall-status", wait_s=0.8)
        print()
        print(status_out.strip())

    print()
    print("========== VERDICT ==========")
    failures = 0

    if pullup == "HIGH":
        print("PASS: open reed reads PULLUP=HIGH on D1")
    elif pullup == "LOW":
        print("WARN: reed closed or stuck — PULLUP=LOW at rest")
        failures += 1
    else:
        print(f"WARN: unexpected baseline PULLUP={pullup}")
        failures += 1

    if "HIGH" in levels and "LOW" in levels:
        print("PASS: both HIGH and LOW seen during hall-watch")
    elif levels == ["HIGH"]:
        print("FAIL: only HIGH — reed never pulls D1 to D0 (LOW)")
        failures += 1
    elif levels == ["LOW"]:
        print("FAIL: only LOW — reed stuck closed")
        failures += 1
    elif not levels:
        print("FAIL: no hall-watch data")
        failures += 1

    if raw_delta > 0:
        print(f"PASS: {raw_delta} raw pulse(s) detected")
    else:
        print("FAIL: no pulses — check magnet or try hall-rising / hall-change")
        failures += 1

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
