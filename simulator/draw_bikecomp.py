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
SCENARIO_ORDER = ("trip", "average", "maximum", "time", "odometer")
GOLDEN_SCENARIOS = SCENARIO_ORDER + ("idle", "paused", "battery_unknown")
SCENARIOS = GOLDEN_SCENARIOS + ("moving",)


def _set_speed_font(lcd):
    if lcd.u8g2_font_dir:
        font_path = Path(lcd.u8g2_font_dir).parent / "ttf" / "Logisoso.ttf"
        if font_path.is_file():
            lcd.font = ImageFont.truetype(str(font_path), 20)
            return
    lcd.setFont(None)


def _ensure_renderer():
    newest_source = max(path.stat().st_mtime for path in RENDERER_SOURCES)
    if not RENDERER.is_file() or RENDERER.stat().st_mtime < newest_source:
        subprocess.run([str(HERE / "build_renderer.sh")], check=True)


def firmware_commands(scenario):
    if scenario not in SCENARIOS:
        raise ValueError(f"Unknown scenario: {scenario}")
    _ensure_renderer()
    result = subprocess.run(
        [str(RENDERER), scenario], check=True, text=True, capture_output=True
    )
    return [line.split("\t") for line in result.stdout.splitlines() if line]


def _execute_command(lcd, command):
    operation = command[0]
    if operation == "FONT":
        _set_speed_font(lcd) if command[1] == "SPEED" else lcd.setFont("5x8")
    elif operation == "TEXT":
        lcd.drawStr(int(command[1]), int(command[2]), command[3])
    elif operation == "FRAME":
        lcd.drawFrame(*(int(value) for value in command[1:5]))
    elif operation == "BOX":
        lcd.drawBox(*(int(value) for value in command[1:5]))
    else:
        raise ValueError(f"Unsupported firmware drawing command: {operation}")


def draw_scenario(lcd, scenario):
    lcd.clearBuffer()
    lcd.setDrawColor(1)
    for command in firmware_commands(scenario):
        _execute_command(lcd, command)
    lcd.sendBuffer()


def draw(lcd):
    scenario = os.environ.get("BIKECOMP_SIM_SCENARIO", "demo")
    if scenario == "demo":
        scenario = SCENARIO_ORDER[int(time.monotonic() // 4) % len(SCENARIO_ORDER)]
    draw_scenario(lcd, scenario)
