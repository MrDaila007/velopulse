#!/usr/bin/env python3
"""Upload Zephyr firmware over USB serial via Adafruit nRF52 bootloader (PIO nrfutil)."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path

# Seeed XIAO nRF52840 + Adafruit UF2 bootloader USB IDs.
_MATCH_VIDS = {0x239A, 0x2886}
_NRF_DEV_TYPE = 0x0052
_SD_REQ = "0x0123"
_DEFAULT_BAUD = 115200


def _repo_root() -> Path:
  return Path(__file__).resolve().parents[2]


def find_adafruit_nrfutil() -> Path:
  override = os.environ.get("ADAFRUIT_NRFUTIL")
  if override:
    path = Path(override).expanduser()
    if path.is_file():
      return path
    raise SystemExit(f"ADAFRUIT_NRFUTIL is not a file: {path}")

  candidates = [
      Path.home() / ".platformio/packages/tool-adafruit-nrfutil/adafruit-nrfutil.py",
      Path("/usr/local/bin/adafruit-nrfutil.py"),
      Path("/usr/bin/adafruit-nrfutil.py"),
  ]
  for candidate in candidates:
    if candidate.is_file():
      return candidate

  raise SystemExit(
      "adafruit-nrfutil not found. Install PlatformIO package tool-adafruit-nrfutil "
      "or set ADAFRUIT_NRFUTIL to adafruit-nrfutil.py")


def _import_pyserial():
  try:
    import serial  # noqa: F401
    import serial.tools.list_ports  # noqa: F401
    return
  except ImportError:
    pass

  nrfutil = find_adafruit_nrfutil()
  site_packages = nrfutil.parent / "site-packages"
  if site_packages.is_dir():
    sys.path.insert(0, str(site_packages))
    try:
      import serial  # noqa: F401
      import serial.tools.list_ports  # noqa: F401
      return
    except ImportError:
      pass

  raise SystemExit("pyserial is required (bundled with adafruit-nrfutil)")


def list_ports():
  _import_pyserial()
  import serial.tools.list_ports

  ports = []
  for info in serial.tools.list_ports.comports():
    if info.vid in _MATCH_VIDS:
      ports.append(info)
  return ports


def port_names() -> set[str]:
  return {info.device for info in list_ports()}


def pick_port(explicit: str | None) -> str | None:
  if explicit:
    return explicit

  env_port = os.environ.get("UPLOAD_PORT")
  if env_port:
    return env_port

  matches = list_ports()
  if len(matches) == 1:
    return matches[0].device
  if not matches:
    return None

  print("Multiple serial ports match XIAO / Adafruit USB IDs:", file=sys.stderr)
  for info in matches:
    print(f"  {info.device}  {info.description}  {info.vid:04X}:{info.pid:04X}",
          file=sys.stderr)
  print("Set UPLOAD_PORT or pass -p.", file=sys.stderr)
  return None


def touch_1200(port: str) -> None:
  _import_pyserial()
  import serial

  print(f"Touch 1200 baud on {port} (enter bootloader)...")
  try:
    with serial.Serial(port, 1200, timeout=0.1) as ser:
      ser.dtr = False
      time.sleep(0.05)
      ser.dtr = True
      time.sleep(0.05)
      ser.dtr = False
  except serial.SerialException as exc:
    print(f"Warning: 1200 touch failed on {port}: {exc}", file=sys.stderr)


def wait_for_port(before: set[str], timeout_s: float) -> str | None:
  deadline = time.monotonic() + timeout_s
  while time.monotonic() < deadline:
    after = port_names()
    new_ports = sorted(after - before)
    if new_ports:
      print(f"Bootloader port: {new_ports[0]}")
      return new_ports[0]

  current = sorted(port_names())
  if len(current) == 1:
    print(f"Using port: {current[0]}")
    return current[0]
  if current:
    print(f"Using port: {current[0]} (no re-enumeration detected)")
    return current[0]
  return None


def make_dfu_package(nrfutil: Path, hex_path: Path, pkg_path: Path) -> None:
  if pkg_path.is_file() and pkg_path.stat().st_mtime >= hex_path.stat().st_mtime:
    return

  pkg_path.parent.mkdir(parents=True, exist_ok=True)
  cmd = [
      sys.executable,
      str(nrfutil),
      "dfu",
      "genpkg",
      "--dev-type",
      hex(_NRF_DEV_TYPE),
      "--sd-req",
      _SD_REQ,
      "--application",
      str(hex_path),
      str(pkg_path),
  ]
  print("Building DFU package:", " ".join(cmd))
  subprocess.run(cmd, check=True)


def run_serial_upload(
    nrfutil: Path,
    pkg_path: Path,
    port: str,
    baud: int,
    touch: bool,
) -> None:
  cmd = [
      sys.executable,
      str(nrfutil),
      "dfu",
      "serial",
      "-p",
      port,
      "-b",
      str(baud),
      "--singlebank",
      "-pkg",
      str(pkg_path),
  ]
  if touch:
    cmd.extend(["-t", "1200"])
  print("Uploading:", " ".join(cmd))
  subprocess.run(cmd, check=True)


def main() -> int:
  parser = argparse.ArgumentParser(
      description="Flash Zephyr hex over Adafruit nRF52 serial DFU (PIO nrfutil)")
  default_build = _repo_root() / "firmware-zephyr" / "build"
  parser.add_argument(
      "--hex",
      type=Path,
      default=default_build / "zephyr" / "zephyr.hex",
      help="Application Intel HEX (default: build/zephyr/zephyr.hex)",
  )
  parser.add_argument(
      "--pkg",
      type=Path,
      default=None,
      help="DFU zip output/cache path (default: next to hex)",
  )
  parser.add_argument("-p", "--port", default=None, help="Serial port (or UPLOAD_PORT)")
  parser.add_argument("-b", "--baud", type=int, default=_DEFAULT_BAUD)
  parser.add_argument(
      "--no-touch",
      action="store_true",
      help="Skip 1200 baud touch (board already in bootloader)",
  )
  parser.add_argument(
      "--wait",
      type=float,
      default=20.0,
      help="Seconds to wait for bootloader port after touch",
  )
  args = parser.parse_args()

  hex_path = args.hex.resolve()
  if not hex_path.is_file():
    raise SystemExit(f"HEX not found: {hex_path} (run make build first)")

  pkg_path = (args.pkg or hex_path.with_name("bikecomp_dfu.zip")).resolve()
  nrfutil = find_adafruit_nrfutil()
  make_dfu_package(nrfutil, hex_path, pkg_path)

  port = pick_port(args.port)
  if port is None:
    raise SystemExit(
        "No upload port found. Connect XIAO USB, or double-tap RESET for bootloader.")

  before = port_names()
  upload_port = port

  if not args.no_touch:
    touch_1200(port)
    upload_port = wait_for_port(before, args.wait) or port

  run_serial_upload(nrfutil, pkg_path, upload_port, args.baud, touch=False)
  print("Upload complete.")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
