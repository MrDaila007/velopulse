#!/usr/bin/env python3
"""Run BikeComp USB serial regression fixtures against a connected board."""

from __future__ import annotations

import argparse
import glob
import re
import sys
import time
from pathlib import Path

from serial_client import open_port

RESULT_RE = re.compile(r"^(OK|FAIL|ERROR)\s+(.*)$")
ROOT = Path(__file__).resolve().parent
DEFAULT_FIXTURE_DIR = ROOT / "fixtures" / "usb"


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
        "Serial port not found. Pass --port /dev/ttyACM0 or connect the board."
    )


def load_fixture(path: Path) -> list[str]:
    lines: list[str] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        lines.append(line)
    return lines


def send_line(port, line: str, wait_s: float) -> str:
    port.write((line + "\n").encode("ascii"))
    time.sleep(wait_s)
    return port.read(8192).decode("utf-8", errors="replace")


def extract_results(output: str) -> list[tuple[str, str]]:
    results: list[tuple[str, str]] = []
    for line in output.splitlines():
        match = RESULT_RE.match(line.strip())
        if match:
            results.append((match.group(1), match.group(2)))
    return results


def run_fixture(port, fixture_path: Path, wait_s: float) -> list[str]:
    failures: list[str] = []
    commands = ["test-on", *load_fixture(fixture_path), "test-off"]
    for command in commands:
        output = send_line(port, command, wait_s)
        results = extract_results(output)
        if not results:
            failures.append(f"{fixture_path.name}: no response for {command!r}")
            continue
        status, message = results[-1]
        if status != "OK":
            failures.append(
                f"{fixture_path.name}: {command!r} -> {status} {message}"
            )
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description="BikeComp USB regression runner")
    parser.add_argument("--port", default=None)
    parser.add_argument(
        "--fixture",
        action="append",
        help="Fixture file or name under tools/fixtures/usb/",
    )
    parser.add_argument(
        "--fixture-dir",
        type=Path,
        default=DEFAULT_FIXTURE_DIR,
        help="Directory with *.txt fixtures",
    )
    parser.add_argument("--wait", type=float, default=0.35)
    args = parser.parse_args()

    if args.fixture:
        fixture_paths = []
        for item in args.fixture:
            path = Path(item)
            if not path.is_file():
                path = args.fixture_dir / item
            if not path.suffix:
                path = path.with_suffix(".txt")
            if not path.is_file():
                raise SystemExit(f"Fixture not found: {item}")
            fixture_paths.append(path)
    else:
        fixture_paths = sorted(args.fixture_dir.glob("*.txt"))

    if not fixture_paths:
        raise SystemExit(f"No fixtures found in {args.fixture_dir}")

    port_path = find_port(args.port)
    print(f"Using serial port: {port_path}")
    all_failures: list[str] = []

    with open_port(port_path) as port:
        time.sleep(0.5)
        port.reset_input_buffer()
        for fixture_path in fixture_paths:
            print(f"RUN {fixture_path.name}")
            failures = run_fixture(port, fixture_path, args.wait)
            if failures:
                all_failures.extend(failures)
                for failure in failures:
                    print(f"  FAIL {failure}")
            else:
                print("  PASS")

    print()
    if all_failures:
        print(f"FAILED ({len(all_failures)} assertion(s))")
        return 1
    print(f"PASSED ({len(fixture_paths)} fixture(s))")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
