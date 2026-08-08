"""Thin u8g2-python-simulator adapter for the production C++ display renderer."""

import os
import subprocess
import time
from pathlib import Path

from PIL import ImageFont

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parent
RENDERER = HERE / ".build" / "firmware_renderer"
RENDERER_SOURCES = (
    HERE / "firmware_renderer.cpp",
    PROJECT / "firmware" / "lib" / "domain" / "display_formatter.cpp",
    PROJECT / "firmware" / "lib" / "domain" / "display_formatter.h",
    PROJECT / "firmware" / "lib" / "domain" / "display_layout.cpp",
    PROJECT / "firmware" / "lib" / "domain" / "display_layout.h",
    PROJECT / "firmware" / "include" / "types.h",
)
SCENARIO_ORDER = ("trip", "average", "maximum", "time", "odometer", "weather_clock",
                  "weather_rain")
GOLDEN_SCENARIOS = SCENARIO_ORDER + ("idle", "paused", "battery_unknown", "low_battery")
BOUNDARY_SCENARIOS = (
    "speed_0",
    "speed_99_9",
    "speed_100",
    "speed_200",
    "battery_empty",
    "battery_full",
    "long_metric",
)
SCENARIOS = GOLDEN_SCENARIOS + ("moving",) + BOUNDARY_SCENARIOS
DISPLAY_HEIGHTS = (32, 64)


def _set_logisoso_font(lcd, size):
    if lcd.u8g2_font_dir:
        font_path = Path(lcd.u8g2_font_dir).parent / "ttf" / "Logisoso.ttf"
        if font_path.is_file():
            lcd.font = ImageFont.truetype(str(font_path), size)
            return
    lcd.setFont(None)


def _set_font(lcd, name):
    if name == "SPEED":
        _set_logisoso_font(lcd, 20)
    elif name == "SPEED_LARGE":
        _set_logisoso_font(lcd, 38)
    elif name == "METRIC_LARGE":
        lcd.setFont("6x13")
    else:
        lcd.setFont("5x8")


def _ensure_renderer():
    newest_source = max(path.stat().st_mtime for path in RENDERER_SOURCES)
    if not RENDERER.is_file() or RENDERER.stat().st_mtime < newest_source:
        subprocess.run([str(HERE / "build_renderer.sh")], check=True)


def firmware_commands(scenario, display_height=64, x_offset=0, y_offset=0):
    if scenario not in SCENARIOS:
        raise ValueError(f"Unknown scenario: {scenario}")
    if display_height not in DISPLAY_HEIGHTS:
        raise ValueError(f"Unsupported display height: {display_height}")
    if x_offset not in (0, 1) or y_offset not in (0, 1):
        raise ValueError("Display offsets must be 0 or 1")
    _ensure_renderer()
    result = subprocess.run(
        [str(RENDERER), scenario, str(display_height), str(x_offset), str(y_offset)],
        check=True,
        text=True,
        capture_output=True,
    )
    return [line.split("\t") for line in result.stdout.splitlines() if line]


def _execute_command(lcd, command):
    operation = command[0]
    if operation == "FONT":
        _set_font(lcd, command[1])
    elif operation == "COLOR":
        lcd.setDrawColor(int(command[1]))
    elif operation == "TEXT":
        lcd.drawStr(int(command[1]), int(command[2]), command[3])
    elif operation == "FRAME":
        lcd.drawFrame(*(int(value) for value in command[1:5]))
    elif operation == "BOX":
        lcd.drawBox(*(int(value) for value in command[1:5]))
    else:
        raise ValueError(f"Unsupported firmware drawing command: {operation}")


def draw_scenario(lcd, scenario, display_height=64):
    lcd.clearBuffer()
    lcd.setDrawColor(1)
    for command in firmware_commands(scenario, display_height):
        _execute_command(lcd, command)
    lcd.sendBuffer()


def draw(lcd):
    scenario = os.environ.get("BIKECOMP_SIM_SCENARIO", "demo")
    display_height = int(os.environ.get("BIKECOMP_DISPLAY_HEIGHT", "64"))
    if scenario == "demo":
        scenario = SCENARIO_ORDER[int(time.monotonic() // 4) % len(SCENARIO_ORDER)]
    draw_scenario(lcd, scenario, display_height)
