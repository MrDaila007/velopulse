#!/usr/bin/env python3
"""Minimal BikeComp Serial helper for calibration and gate scripts."""

from __future__ import annotations

import argparse
import sys
import time

try:
    import serial
except ImportError as exc:  # pragma: no cover - host tooling
    raise SystemExit("pyserial required: pip install pyserial") from exc


def open_port(path: str, baud: int = 115200) -> serial.Serial:
    return serial.Serial(path, baud, timeout=0.5)


def send_command(port: serial.Serial, command: str, wait_s: float = 1.5) -> str:
    port.reset_input_buffer()
    port.write((command.strip() + "\n").encode("ascii"))
    time.sleep(wait_s)
    return port.read(8192).decode("utf-8", errors="replace")


def main() -> int:
    parser = argparse.ArgumentParser(description="Send a BikeComp Serial command")
    parser.add_argument("--port", default="/dev/ttyACM1")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("command", help="e.g. selftest, ambient-raw, dump-config")
    parser.add_argument("--wait", type=float, default=1.5)
    args = parser.parse_args()

    with open_port(args.port, args.baud) as port:
        output = send_command(port, args.command, args.wait)
    sys.stdout.write(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
