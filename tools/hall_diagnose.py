#!/usr/bin/env python3
"""Hall sensor diagnostic helper over BikeComp Serial."""

from __future__ import annotations

import argparse
import re
import sys
import time

from serial_client import open_port, send_command

HALL_RE = re.compile(
    r"Hall: pin=D0\+D1.*level=(HIGH|LOW), .*edge=(\w+), raw_pulses=(\d+), "
    r"accepted=(\d+), debounce_rej=(\d+), overspeed_rej=(\d+), "
    r"isr_ovf=(\d+), ride=(\w+), revolutions=(\d+)"
    r"(?:, speed_x100=(\d+), max_speed_x100=(\d+), gap_corr=(\d+), "
    r"last_interval_us=(\d+))?"
)


def parse_hall(text: str) -> dict[str, str | int] | None:
    match = HALL_RE.search(text)
    if not match:
        return None
    result = {
        "level": match.group(1),
        "edge": match.group(2),
        "raw_pulses": int(match.group(3)),
        "accepted": int(match.group(4)),
        "debounce_rej": int(match.group(5)),
        "overspeed_rej": int(match.group(6)),
        "isr_ovf": int(match.group(7)),
        "ride": match.group(8),
        "revolutions": int(match.group(9)),
    }
    if match.group(10) is not None:
        result["speed_x100"] = int(match.group(10))
        result["max_speed_x100"] = int(match.group(11))
        result["gap_corr"] = int(match.group(12))
        result["last_interval_us"] = int(match.group(13))
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Hall sensor diagnostics")
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--watch-seconds", type=float, default=20.0)
    args = parser.parse_args()

    with open_port(args.port) as port:
        config_out = send_command(port, "dump-config", wait_s=1.5)
        edge_match = re.search(r"active_edge=(\d+)", config_out)
        debounce_match = re.search(r"debounce_ms=(\d+)", config_out)
        edge = edge_match.group(1) if edge_match else "?"
        debounce = debounce_match.group(1) if debounce_match else "?"

        status_out = send_command(port, "hall-status", wait_s=0.8)
        status = parse_hall(status_out)
        if not status:
            print(status_out, file=sys.stderr)
            print("FAIL: could not parse hall-status", file=sys.stderr)
            return 1

        print("Hall diagnostic")
        print(f"  config: active_edge={edge} (0=FALLING 1=RISING 2=CHANGE), debounce_ms={debounce}")
        print(f"  D1 sense: {status['level']} (magnet away expects HIGH, near expects LOW)")
        print(f"  counters: raw={status['raw_pulses']} accepted={status['accepted']} "
              f"debounce_rej={status['debounce_rej']} overspeed_rej={status['overspeed_rej']}")
        print(f"  ride={status['ride']} revolutions={status['revolutions']}")
        if "speed_x100" in status:
            print(
                "  speed: current="
                f"{status['speed_x100'] / 100:.1f} km/h max="
                f"{status['max_speed_x100'] / 100:.1f} km/h "
                f"gap_corr={status['gap_corr']} "
                f"last_interval_us={status['last_interval_us']}"
            )
        print()
        print(f"Watching {args.watch_seconds:.0f}s — move magnet near/away reed (D0<->D1)...")
        port.reset_input_buffer()
        port.write(b"hall-watch\n")
        time.sleep(0.3)
        start = status["raw_pulses"]
        start_level = status["level"]
        text = ""
        deadline = time.time() + args.watch_seconds
        levels: set[str] = set()
        while time.time() < deadline:
            text += port.read(4096).decode("utf-8", errors="replace")
            time.sleep(0.15)
        port.write(b"hall-stop\n")

    lines = [line for line in text.splitlines() if line.startswith("Hall:")]
    end = parse_hall(lines[-1]) if lines else status
    for line in lines:
        parsed = parse_hall(line)
        if parsed:
            levels.add(str(parsed["level"]))

    print()
    print("Watch summary")
    print(f"  D1 levels seen: {sorted(levels) if levels else [start_level]}")
    print(f"  raw_pulses: {start} -> {end['raw_pulses']} (delta {end['raw_pulses'] - start})")
    print(f"  accepted: {end['accepted']} ride={end['ride']} revolutions={end['revolutions']}")

    if end["raw_pulses"] == start:
        print()
        print("NO pulses detected. Reed / D0+D1 checks:")
        print("  1. Wiring: one reed contact -> D0, other -> D1 (no 3V3)")
        print("  2. Magnet away: gpio-probe should show PULLUP=HIGH")
        print("  3. Magnet near: D1=LOW; move near -> FALLING pulse (active_edge=0)")
        print("  4. If always LOW: reed stuck closed or wrong pins")
        print("  5. Serial: hall-rising / hall-falling / hall-watch")
        return 2

    if end["accepted"] == 0 and end["raw_pulses"] > start:
        print()
        print("ISR fires but filter rejects all pulses — try slower trigger or check debounce.")
        return 3

    print()
    print("PASS: Hall path responds")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
