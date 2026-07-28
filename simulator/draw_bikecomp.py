"""BikeComp 128x32 OLED renderer for u8g2-python-simulator."""

import os
import time
from pathlib import Path

from PIL import ImageFont

SCENARIOS = {
    "idle": {"speed_x100": 0, "trip_distance_mm": 0, "state": "IDLE"},
    "moving": {"speed_x100": 2480, "trip_distance_mm": 18_420_000, "state": "MOV"},
    "paused": {"speed_x100": 0, "trip_distance_mm": 18_420_000, "state": "PAUSE"},
}
SCENARIO_ORDER = ("idle", "moving", "paused")


def _set_speed_font(lcd):
    if lcd.u8g2_font_dir:
        font_path = Path(lcd.u8g2_font_dir).parent / "ttf" / "Logisoso.ttf"
        if font_path.is_file():
            lcd.font = ImageFont.truetype(str(font_path), 20)
            return
    lcd.setFont(None)


def _set_small_font(lcd):
    lcd.setFont("5x8")


def format_frame(state):
    speed_x100 = int(state["speed_x100"])
    trip_mm = int(state["trip_distance_mm"])
    speed = f"{speed_x100 // 100}.{(speed_x100 % 100) // 10}"
    lower = f"{state['state']}  TRIP {trip_mm // 1_000_000}.{(trip_mm // 10_000) % 100:02d}"
    return speed, lower


def draw_scenario(lcd, scenario):
    if scenario not in SCENARIOS:
        raise ValueError(f"Unknown scenario: {scenario}")
    speed, lower = format_frame(SCENARIOS[scenario])

    lcd.clearBuffer()
    lcd.setDrawColor(1)
    _set_speed_font(lcd)
    lcd.drawStr(0, 21, speed)
    _set_small_font(lcd)
    lcd.drawStr(88, 20, "km/h")
    lcd.drawStr(0, 31, lower)
    lcd.sendBuffer()


def draw(lcd):
    scenario = os.environ.get("BIKECOMP_SIM_SCENARIO", "demo")
    if scenario == "demo":
        scenario = SCENARIO_ORDER[int(time.monotonic() // 4) % len(SCENARIO_ORDER)]
    draw_scenario(lcd, scenario)
