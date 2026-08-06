#!/usr/bin/env python3
"""Read and write BikeComp config + odometer fields over USB serial.

Supports full profile migration (pull/push) and selective field updates (show/set/patch).
JSON matches the mobile app's firmware-migration backup schema.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

try:
    import serial
except ImportError as exc:  # pragma: no cover - host tooling
    raise SystemExit("pyserial required: pip install pyserial") from exc

SCHEMA_VERSION = 1
WIRE_V1_HEX_LEN = 96
FW_VERSION_RE = re.compile(r"BikeComp FW ([^\r\n]+)")
WIRE_V1_RE = re.compile(r"^\s*wire_v1=([0-9A-Fa-f]+)\s*$", re.MULTILINE)
ODO_MM_RE = re.compile(r"\bodo_mm=(\d+)")
OK_LOAD_CONFIG_RE = re.compile(r"OK load-config")
OK_SET_ODO_RE = re.compile(r"OK set-odo-mm")
INT_FIELD_RE = re.compile(r"^\s*([a-z_]+)=(\d+)\s*$", re.MULTILINE)
HEX_FIELD_RE = re.compile(r"^\s*enabled_pages_mask=0x([0-9A-Fa-f]+)\s*$", re.MULTILINE)
FLAGS_RE = re.compile(r"^\s*flags=([01]{8})\s*$", re.MULTILINE)
NAME_RE = re.compile(r"^\s*device_name=(.+)\s*$", re.MULTILINE)
PAGE_ORDER_RE = re.compile(r"^\s*page_order=([0-9,]+)\s*$", re.MULTILINE)

CONFIG_INT_FIELDS = (
    "structVersion",
    "flags",
    "wheelCircumferenceMm",
    "maxSpeedKmh",
    "stopTimeoutS",
    "displayTimeoutS",
    "deepSleepTimeoutS",
    "brightnessPct",
    "pageSwitchPeriodS",
    "enabledPagesMask",
    "lowBatteryPct",
    "odometerSaveIntervalM",
    "smoothingWindow",
    "debounceMs",
    "activeEdge",
    "pinnedPage",
    "battCalScalePermille",
    "battCalOffsetMv",
    "reservedPage",
    "reserved",
)

CONFIG_BOOL_FIELDS = {
    "smoothingEnabled": 0x01,
    "autoPageSwitch": 0x02,
    "displayAutoOff": 0x04,
    "bleAlwaysAdvertise": 0x08,
    "unitsImperial": 0x10,
    "sensorInvert": 0x20,
    "powerSaveMode": 0x40,
    "deepSleepEnabled": 0x80,
}

PROFILE_SCALAR_FIELDS = ("odometerMm", "odometerM")

FIELD_ALIASES = {
    "wheel_circumference_mm": "wheelCircumferenceMm",
    "max_speed_kmh": "maxSpeedKmh",
    "stop_timeout_s": "stopTimeoutS",
    "display_timeout_s": "displayTimeoutS",
    "deep_sleep_timeout_s": "deepSleepTimeoutS",
    "brightness_pct": "brightnessPct",
    "page_switch_period_s": "pageSwitchPeriodS",
    "enabled_pages_mask": "enabledPagesMask",
    "low_battery_pct": "lowBatteryPct",
    "odometer_save_interval_m": "odometerSaveIntervalM",
    "smoothing_window": "smoothingWindow",
    "debounce_ms": "debounceMs",
    "active_edge": "activeEdge",
    "pinned_page": "pinnedPage",
    "batt_cal_scale_permille": "battCalScalePermille",
    "batt_cal_offset_mv": "battCalOffsetMv",
    "page_order": "pageOrder",
    "device_name": "deviceName",
    "odo_mm": "odometerMm",
    "odometer_mm": "odometerMm",
    "odometer_m": "odometerM",
}


def open_port(path: str, baud: int = 115200) -> serial.Serial:
    port = serial.Serial(path, baud, timeout=0.5)
    port.dtr = True
    port.rts = False
    return port


def wait_for_device(port: serial.Serial, wait_s: float, attempts: int = 20) -> serial.Serial:
    last_text = ""
    for _ in range(attempts):
        try:
            last_text = send_command(port, "status", wait_s)
            if "odo_mm=" in last_text or "OK status" in last_text:
                return port
        except (OSError, serial.SerialException):
            port = reopen_port(port)
        time.sleep(0.75)
    raise RuntimeError(f"device not ready (status failed): {last_text.strip()[-300:]}")


def command_succeeded(text: str, pattern: re.Pattern[str]) -> bool:
    return pattern.search(text) is not None


def drain(port: serial.Serial, wait_s: float = 0.2) -> str:
    time.sleep(wait_s)
    chunks: list[str] = []
    try:
        while port.in_waiting:
            chunks.append(port.read(port.in_waiting).decode("utf-8", errors="replace"))
            time.sleep(0.05)
    except OSError:
        return ""
    return "".join(chunks)


def reopen_port(port: serial.Serial) -> serial.Serial:
    path = port.port
    baud = port.baudrate
    try:
        port.close()
    except OSError:
        pass
    time.sleep(1.0)
    return open_port(path, baud)


def send_command(port: serial.Serial, command: str, wait_s: float = 1.5) -> str:
    port.reset_input_buffer()
    port.write((command.strip() + "\n").encode("ascii"))
    return drain(port, wait_s)


def canonical_field_name(name: str) -> str:
    key = name.strip()
    return FIELD_ALIASES.get(key, key)


def parse_bool(value: str) -> bool:
    lowered = value.strip().lower()
    if lowered in {"1", "true", "yes", "on"}:
        return True
    if lowered in {"0", "false", "no", "off"}:
        return False
    raise ValueError(f"expected boolean, got {value!r}")


def parse_field_value(field: str, raw: str) -> Any:
    field = canonical_field_name(field)
    if field in PROFILE_SCALAR_FIELDS:
        return int(raw)
    if field == "pageOrder":
        return [int(part.strip()) for part in raw.split(",") if part.strip()]
    if field == "deviceName":
        return raw
    if field in CONFIG_BOOL_FIELDS:
        return parse_bool(raw)
    if field in CONFIG_INT_FIELDS:
        if field == "enabledPagesMask" and raw.lower().startswith("0x"):
            return int(raw, 16)
        return int(raw)
    raise ValueError(f"unknown field {field!r}")


def parse_assignments(items: list[str]) -> dict[str, Any]:
    patch: dict[str, Any] = {}
    config_patch: dict[str, Any] = {}
    for item in items:
        if "=" not in item:
            raise ValueError(f"expected key=value, got {item!r}")
        key, value = item.split("=", 1)
        key = canonical_field_name(key)
        parsed = parse_field_value(key, value)
        if key in PROFILE_SCALAR_FIELDS:
            patch[key] = parsed
        elif key in CONFIG_BOOL_FIELDS:
            config_patch[key] = parsed
        elif key in CONFIG_INT_FIELDS or key in {"pageOrder", "deviceName"}:
            config_patch[key] = parsed
        else:
            raise ValueError(f"unknown field {key!r}")
    if config_patch:
        patch["config"] = config_patch
    return patch


def flags_from_dump(flags_text: str) -> int:
    bits = [bit == "1" for bit in flags_text]
    while len(bits) < 8:
        bits.append(False)
    value = 0
    for index, enabled in enumerate(bits):
        if enabled:
            value |= 1 << index
    return value


def flags_to_bools(flags: int) -> dict[str, bool]:
    return {
        name: (flags & mask) != 0 for name, mask in CONFIG_BOOL_FIELDS.items()
    }


def apply_bool_flags(config: dict[str, Any], bool_patch: dict[str, Any]) -> None:
    flags = int(config.get("flags", 15))
    for name, enabled in bool_patch.items():
        if name not in CONFIG_BOOL_FIELDS:
            continue
        mask = CONFIG_BOOL_FIELDS[name]
        if enabled:
            flags |= mask
        else:
            flags &= ~mask
    config["flags"] = flags
    config.update(flags_to_bools(flags))


def config_from_dump(text: str) -> dict[str, Any]:
    flags_match = FLAGS_RE.search(text)
    if not flags_match:
        raise ValueError("dump-config missing flags=")
    name_match = NAME_RE.search(text)
    page_order_match = PAGE_ORDER_RE.search(text)
    ints = {key: int(value) for key, value in INT_FIELD_RE.findall(text)}
    enabled_mask_match = HEX_FIELD_RE.search(text)
    if enabled_mask_match:
        ints["enabled_pages_mask"] = int(enabled_mask_match.group(1), 16)

    flags = flags_from_dump(flags_match.group(1))
    config = {
        "structVersion": 1,
        "flags": flags,
        "wheelCircumferenceMm": ints["wheel_circumference_mm"],
        "maxSpeedKmh": ints["max_speed_kmh"],
        "stopTimeoutS": ints["stop_timeout_s"],
        "displayTimeoutS": ints["display_timeout_s"],
        "deepSleepTimeoutS": ints["deep_sleep_timeout_s"],
        "brightnessPct": ints["brightness_pct"],
        "pageSwitchPeriodS": ints["page_switch_period_s"],
        "enabledPagesMask": ints["enabled_pages_mask"],
        "lowBatteryPct": ints["low_battery_pct"],
        "odometerSaveIntervalM": ints["odometer_save_interval_m"],
        "smoothingWindow": ints["smoothing_window"],
        "debounceMs": ints["debounce_ms"],
        "activeEdge": ints["active_edge"],
        "pinnedPage": ints["pinned_page"],
        "battCalScalePermille": ints["batt_cal_scale_permille"],
        "battCalOffsetMv": ints["batt_cal_offset_mv"],
        "pageOrder": [int(part) for part in page_order_match.group(1).split(",")]
        if page_order_match
        else [0, 1, 2, 3, 4],
        "reservedPage": 0,
        "deviceName": name_match.group(1).strip() if name_match else "BikeComp",
        "reserved": 0,
    }
    config.update(flags_to_bools(flags))
    return config


def encode_wire_v1(config: dict[str, Any]) -> str:
    payload = bytearray(48)
    payload[0] = 1
    payload[1] = int(config.get("flags", 15)) & 0xFF

    def write_u16(offset: int, value: int) -> None:
        payload[offset] = value & 0xFF
        payload[offset + 1] = (value >> 8) & 0xFF

    write_u16(2, int(config["wheelCircumferenceMm"]))
    payload[4] = int(config["maxSpeedKmh"]) & 0xFF
    payload[5] = int(config["stopTimeoutS"]) & 0xFF
    write_u16(6, int(config["displayTimeoutS"]))
    write_u16(8, int(config["deepSleepTimeoutS"]))
    payload[10] = int(config["brightnessPct"]) & 0xFF
    payload[11] = int(config["pageSwitchPeriodS"]) & 0xFF
    payload[12] = int(config["enabledPagesMask"]) & 0xFF
    payload[13] = int(config["lowBatteryPct"]) & 0xFF
    write_u16(14, int(config["odometerSaveIntervalM"]))
    payload[16] = int(config["smoothingWindow"]) & 0xFF
    payload[17] = int(config["debounceMs"]) & 0xFF
    payload[18] = int(config["activeEdge"]) & 0xFF
    payload[19] = int(config["pinnedPage"]) & 0xFF
    write_u16(20, int(config["battCalScalePermille"]))
    offset_mv = int(config["battCalOffsetMv"])
    write_u16(22, offset_mv & 0xFFFF)
    page_order = list(config.get("pageOrder", [0, 1, 2, 3, 4]))
    for index, page in enumerate(page_order[:5]):
        payload[24 + index] = int(page) & 0xFF
    device_name = str(config.get("deviceName", "BikeComp"))
    for index, char in enumerate(device_name[:15]):
        payload[30 + index] = ord(char)
    return payload.hex()


def parse_dump_config(text: str) -> tuple[dict[str, Any], str]:
    wire_match = WIRE_V1_RE.search(text)
    if not wire_match:
        raise ValueError("dump-config missing wire_v1=")
    wire_v1 = wire_match.group(1).lower()
    if len(wire_v1) != WIRE_V1_HEX_LEN:
        raise ValueError(f"wire_v1 must be {WIRE_V1_HEX_LEN} hex chars, got {len(wire_v1)}")
    return config_from_dump(text), wire_v1


def parse_status_odo_mm(text: str) -> int:
    match = ODO_MM_RE.search(text)
    if not match:
        raise ValueError("status missing odo_mm=")
    return int(match.group(1))


def parse_fw_version(text: str) -> str | None:
    match = FW_VERSION_RE.search(text)
    return match.group(1).strip() if match else None


def build_profile(
    dump_text: str,
    status_text: str,
    boot_text: str = "",
    *,
    source: str = "usb",
) -> dict[str, Any]:
    config, wire_v1 = parse_dump_config(dump_text)
    odometer_mm = parse_status_odo_mm(status_text)
    device_name = str(config.get("deviceName", "BikeComp"))
    return {
        "schema": SCHEMA_VERSION,
        "source": source,
        "deviceId": "usb",
        "deviceName": device_name,
        "config": config,
        "odometerM": odometer_mm // 1000,
        "odometerMm": odometer_mm,
        "wireV1": wire_v1,
        "savedAt": datetime.now(timezone.utc).isoformat(),
        "fwVersion": parse_fw_version(boot_text or dump_text or status_text),
    }


def normalize_profile(data: dict[str, Any]) -> dict[str, Any]:
    if data.get("schema") != SCHEMA_VERSION:
        raise ValueError(f"unsupported schema {data.get('schema')!r}")
    config = data.get("config")
    if not isinstance(config, dict):
        raise ValueError("profile missing config object")
    wire_v1 = data.get("wireV1")
    if not isinstance(wire_v1, str) or len(wire_v1) != WIRE_V1_HEX_LEN:
        wire_v1 = encode_wire_v1(config)
    odometer_mm = data.get("odometerMm")
    if odometer_mm is None:
        odometer_mm = int(data.get("odometerM", 0)) * 1000
    return {
        "config": config,
        "wireV1": wire_v1.lower(),
        "odometerMm": int(odometer_mm),
    }


def merge_profile(base: dict[str, Any], patch: dict[str, Any]) -> dict[str, Any]:
    merged = json.loads(json.dumps(base))
    config_patch = patch.get("config")
    if isinstance(config_patch, dict):
        bool_patch = {
            key: value
            for key, value in config_patch.items()
            if key in CONFIG_BOOL_FIELDS and isinstance(value, bool)
        }
        scalar_patch = {
            key: value
            for key, value in config_patch.items()
            if key not in CONFIG_BOOL_FIELDS
        }
        merged["config"].update(scalar_patch)
        if bool_patch:
            apply_bool_flags(merged["config"], bool_patch)
        merged["wireV1"] = encode_wire_v1(merged["config"])
        merged["deviceName"] = merged["config"].get("deviceName", merged.get("deviceName"))
    if "odometerMm" in patch:
        merged["odometerMm"] = int(patch["odometerMm"])
        merged["odometerM"] = merged["odometerMm"] // 1000
    elif "odometerM" in patch:
        merged["odometerM"] = int(patch["odometerM"])
        merged["odometerMm"] = merged["odometerM"] * 1000
    merged["savedAt"] = datetime.now(timezone.utc).isoformat()
    return merged


def config_changed(before: dict[str, Any], after: dict[str, Any]) -> bool:
    return encode_wire_v1(before) != encode_wire_v1(after)


def pull_profile(port: serial.Serial, wait_s: float) -> dict[str, Any]:
    port = wait_for_device(port, wait_s)
    boot_text = drain(port, 0.2)
    dump_text = send_command(port, "dump-config", wait_s)
    if not command_succeeded(dump_text, re.compile(r"OK dump-config")):
        raise RuntimeError("dump-config failed")
    status_text = send_command(port, "status", wait_s)
    if "odo_mm=" not in status_text:
        raise RuntimeError("status failed")
    return build_profile(dump_text, status_text, boot_text)


def write_config(port: serial.Serial, wire_v1: str, wait_s: float) -> serial.Serial:
    port = wait_for_device(port, wait_s)
    last_text = ""
    for attempt in range(5):
        port.reset_input_buffer()
        port.write(f"load-config\n{wire_v1}\n".encode("ascii"))
        last_text = drain(port, wait_s + float(attempt))
        if command_succeeded(last_text, OK_LOAD_CONFIG_RE):
            return port
        time.sleep(0.75)
    raise RuntimeError(f"load-config failed: {last_text.strip()[-500:]}")


def write_odometer(port: serial.Serial, odometer_mm: int, wait_s: float) -> None:
    last_text = ""
    for attempt in range(5):
        last_text = send_command(port, f"set-odo-mm {odometer_mm}", wait_s + float(attempt))
        if command_succeeded(last_text, OK_SET_ODO_RE):
            return
        time.sleep(0.5)
    raise RuntimeError(f"set-odo-mm failed: {last_text.strip()[-500:]}")


def profile_diff(before: dict[str, Any], after: dict[str, Any]) -> list[str]:
    normalized = normalize_profile(after)
    changed: list[str] = []
    if config_changed(before["config"], normalized["config"]):
        changed.append("config")
    if int(before.get("odometerMm", 0)) != normalized["odometerMm"]:
        changed.append("odometer")
    return changed


def apply_profile(
    port: serial.Serial,
    before: dict[str, Any],
    after: dict[str, Any],
    wait_s: float,
) -> list[str]:
    normalized = normalize_profile(after)
    changed = profile_diff(before, after)
    if "config" in changed:
        write_config(port, normalized["wireV1"], wait_s)
    if "odometer" in changed:
        write_odometer(port, normalized["odometerMm"], wait_s)
    return changed


def push_profile(port: serial.Serial, profile: dict[str, Any], wait_s: float) -> None:
    normalized = normalize_profile(profile)
    port = write_config(port, normalized["wireV1"], wait_s)
    write_odometer(port, normalized["odometerMm"], wait_s)


def format_show(profile: dict[str, Any]) -> str:
    lines = ["BikeComp profile:"]
    if profile.get("fwVersion"):
        lines.append(f"  fwVersion={profile['fwVersion']}")
    lines.append(f"  odometerMm={profile.get('odometerMm', 0)}")
    lines.append(f"  odometerM={profile.get('odometerM', 0)}")
    config = profile.get("config", {})
    for key in (
        "deviceName",
        "wheelCircumferenceMm",
        "maxSpeedKmh",
        "stopTimeoutS",
        "displayTimeoutS",
        "deepSleepTimeoutS",
        "brightnessPct",
        "pageSwitchPeriodS",
        "enabledPagesMask",
        "lowBatteryPct",
        "odometerSaveIntervalM",
        "smoothingWindow",
        "debounceMs",
        "activeEdge",
        "pinnedPage",
        "battCalScalePermille",
        "battCalOffsetMv",
        "pageOrder",
    ):
        if key in config:
            lines.append(f"  {key}={config[key]!r}")
    for name in CONFIG_BOOL_FIELDS:
        if name in config:
            lines.append(f"  {name}={config[name]!r}")
    return "\n".join(lines) + "\n"


def cmd_show(args: argparse.Namespace) -> int:
    with open_port(args.port, args.baud) as port:
        profile = pull_profile(port, args.wait)
    sys.stdout.write(format_show(profile))
    return 0


def cmd_pull(args: argparse.Namespace) -> int:
    with open_port(args.port, args.baud) as port:
        profile = pull_profile(port, args.wait)
    output = json.dumps(profile, indent=2, ensure_ascii=False) + "\n"
    if args.output:
        Path(args.output).write_text(output, encoding="utf-8")
        print(f"Saved profile to {args.output}", file=sys.stderr)
    else:
        sys.stdout.write(output)
    return 0


def _require_input_file(args: argparse.Namespace) -> Path:
    if not args.input_file:
        raise SystemExit("profile JSON file required (positional FILE or -i/--input)")
    return Path(args.input_file)


def cmd_push(args: argparse.Namespace) -> int:
    profile = json.loads(_require_input_file(args).read_text(encoding="utf-8"))
    with open_port(args.port, args.baud) as port:
        push_profile(port, profile, args.wait)
    print(
        f"Restored config and odometer ({profile.get('odometerMm', profile.get('odometerM', 0))} mm)",
        file=sys.stderr,
    )
    return 0


def cmd_set(args: argparse.Namespace) -> int:
    patch = parse_assignments(args.fields)
    with open_port(args.port, args.baud) as port:
        current = pull_profile(port, args.wait)
        updated = merge_profile(current, patch)
        changed = apply_profile(port, current, updated, args.wait)
    if not changed:
        print("No changes", file=sys.stderr)
    else:
        print(f"Updated: {', '.join(changed)}", file=sys.stderr)
    return 0


def cmd_patch(args: argparse.Namespace) -> int:
    patch = json.loads(_require_input_file(args).read_text(encoding="utf-8"))
    with open_port(args.port, args.baud) as port:
        current = pull_profile(port, args.wait)
        updated = merge_profile(current, patch)
        changed = apply_profile(port, current, updated, args.wait)
    if not changed:
        print("No changes", file=sys.stderr)
    else:
        print(f"Updated: {', '.join(changed)}", file=sys.stderr)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Read/write BikeComp config and odometer fields over USB serial",
    )
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("-p", "--port", default="/dev/ttyACM0")
    common.add_argument("--baud", type=int, default=115200)
    common.add_argument("--wait", type=float, default=2.0)
    subparsers = parser.add_subparsers(dest="command", required=True)

    show = subparsers.add_parser(
        "show", parents=[common], help="print current fields from device"
    )
    show.set_defaults(func=cmd_show)

    pull = subparsers.add_parser(
        "pull", parents=[common], help="read full profile to JSON"
    )
    pull.add_argument("-o", "--output", help="write JSON file (default: stdout)")
    pull.set_defaults(func=cmd_pull)

    push = subparsers.add_parser(
        "push", parents=[common], help="write full profile from JSON file"
    )
    push.add_argument(
        "input_file",
        nargs="?",
        metavar="FILE",
        help="profile JSON file (or use -i/--input)",
    )
    push.add_argument("-i", "--input", dest="input_file", help="profile JSON file")
    push.set_defaults(func=cmd_push)

    set_cmd = subparsers.add_parser(
        "set",
        parents=[common],
        help="overwrite selected fields (reads device, merges, writes back)",
    )
    set_cmd.add_argument(
        "fields",
        nargs="+",
        metavar="FIELD=VALUE",
        help="e.g. wheelCircumferenceMm=2100 odometerM=42 unitsImperial=true",
    )
    set_cmd.set_defaults(func=cmd_set)

    patch = subparsers.add_parser(
        "patch",
        parents=[common],
        help="merge partial JSON over device state and write changed fields",
    )
    patch.add_argument(
        "input_file",
        nargs="?",
        metavar="FILE",
        help="partial profile JSON (or use -i/--input)",
    )
    patch.add_argument("-i", "--input", dest="input_file", help="partial profile JSON")
    patch.set_defaults(func=cmd_patch)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
