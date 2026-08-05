#!/usr/bin/env python3
"""Unit tests for bike_profile parsing and encoding."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))

from bike_profile import (  # noqa: E402
    build_profile,
    encode_wire_v1,
    merge_profile,
    normalize_profile,
    parse_assignments,
    parse_dump_config,
    parse_status_odo_mm,
    profile_diff,
)

SAMPLE_DUMP = """\
Config:
  wheel_circumference_mm=2100
  max_speed_kmh=100
  stop_timeout_s=3
  display_timeout_s=60
  deep_sleep_timeout_s=900
  brightness_pct=60
  page_switch_period_s=4
  enabled_pages_mask=0x1f
  low_battery_pct=20
  odometer_save_interval_m=500
  smoothing_window=3
  debounce_ms=3
  active_edge=0
  pinned_page=0
  batt_cal_scale_permille=1000
  batt_cal_offset_mv=0
  page_order=0,1,2,3,4
  device_name=BikeComp-XXXX
  flags=11110000
  wire_v1=010f340864033c0084033c041f14f40103030000e803000000010203040042696b65436f6d702d585858580000000000
OK dump-config
"""

SAMPLE_STATUS = (
    "Status: speed_x100=0 avg_speed_x100=0 max_speed_x100=0 trip_mm=0 "
    "odo_mm=12345678 rev=0 total_rev=0 moving_ms=0 ride=IDLE battery_mv=4200 "
    "battery_pct=80 battery_valid=1 usb=1 display=bright hall=LOW raw_pulses=0 "
    "accepted=0 debounce_rej=0 overspeed_rej=0 power_mode=normal deep_sleep_armed=0 "
    "usb_test=0 clock=-- clock_valid=0 clock_stale=0 weather_temp=-- weather_rain=-- "
    "weather_valid=0 weather_stale=0\n"
)


class BikeProfileTest(unittest.TestCase):
    def test_parse_dump_config(self) -> None:
        config, wire_v1 = parse_dump_config(SAMPLE_DUMP)
        self.assertEqual(config["deviceName"], "BikeComp-XXXX")
        self.assertEqual(config["wheelCircumferenceMm"], 2100)
        self.assertEqual(len(wire_v1), 96)

    def test_parse_status_odo_mm(self) -> None:
        self.assertEqual(parse_status_odo_mm(SAMPLE_STATUS), 12345678)

    def test_build_profile(self) -> None:
        profile = build_profile(SAMPLE_DUMP, SAMPLE_STATUS)
        self.assertEqual(profile["schema"], 1)
        self.assertEqual(profile["odometerMm"], 12345678)
        self.assertEqual(profile["odometerM"], 12345)
        self.assertEqual(profile["config"]["deviceName"], "BikeComp-XXXX")

    def test_normalize_profile_from_mobile_backup(self) -> None:
        mobile_backup = {
            "schema": 1,
            "deviceId": "abc",
            "deviceName": "BikeComp",
            "config": build_profile(SAMPLE_DUMP, SAMPLE_STATUS)["config"],
            "odometerM": 42,
            "savedAt": "2026-08-02T10:00:00Z",
        }
        normalized = normalize_profile(mobile_backup)
        self.assertEqual(normalized["odometerMm"], 42000)
        self.assertEqual(len(normalized["wireV1"]), 96)

    def test_encode_wire_v1_round_trip(self) -> None:
        config, wire_v1 = parse_dump_config(SAMPLE_DUMP)
        self.assertEqual(encode_wire_v1(config), wire_v1)

    def test_parse_assignments_and_merge(self) -> None:
        profile = build_profile(SAMPLE_DUMP, SAMPLE_STATUS)
        patch = parse_assignments(
            ["wheelCircumferenceMm=2150", "odometerM=99", "unitsImperial=true"]
        )
        merged = merge_profile(profile, patch)
        self.assertEqual(merged["config"]["wheelCircumferenceMm"], 2150)
        self.assertTrue(merged["config"]["unitsImperial"])
        self.assertEqual(merged["odometerMm"], 99000)

    def test_profile_diff_detects_changed_sections(self) -> None:
        profile = build_profile(SAMPLE_DUMP, SAMPLE_STATUS)
        odo_only = merge_profile(profile, {"odometerM": 50})
        self.assertEqual(profile_diff(profile, odo_only), ["odometer"])

        config_only = merge_profile(profile, {"config": {"wheelCircumferenceMm": 2150}})
        self.assertEqual(profile_diff(profile, config_only), ["config"])

        same = merge_profile(profile, {})
        self.assertEqual(profile_diff(profile, same), [])


if __name__ == "__main__":
    unittest.main()
