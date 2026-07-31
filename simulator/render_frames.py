#!/usr/bin/env python3
"""Render deterministic BikeComp screenshots using the pinned upstream simulator."""

import argparse
import sys
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
UPSTREAM = HERE / ".vendor" / "u8g2-python-simulator"
U8G2_ROOT = HERE / ".vendor" / "u8g2"
sys.path.insert(0, str(UPSTREAM))

from u8g2_sim import U8G2SimLCD  # noqa: E402
from draw_bikecomp import SCENARIOS, SCENARIO_ORDER, draw_scenario  # noqa: E402


def render_raw(scenario, display_height=64):
    lcd = U8G2SimLCD(
        128,
        display_height,
        scale=1,
        title="BikeComp OLED",
        u8g2_dir=str(U8G2_ROOT),
    )
    lcd._show_fps = False
    draw_scenario(lcd, scenario, display_height)
    image = lcd.img.copy()
    lcd.root.destroy()
    return image


def save_scaled(image, output, scale):
    output.parent.mkdir(parents=True, exist_ok=True)
    image.resize(
        (image.width * scale, image.height * scale), Image.Resampling.NEAREST
    ).save(output)


def render_contact_sheet(output, scale, display_height):
    frames = [render_raw(name, display_height) for name in SCENARIO_ORDER]
    sheet = Image.new("L", (128, display_height * len(frames)), 0)
    for index, frame in enumerate(frames):
        sheet.paste(frame, (0, index * display_height))
    save_scaled(sheet, output, scale)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--scenario", choices=tuple(SCENARIOS))
    parser.add_argument("--display-height", type=int, choices=(32, 64), default=64)
    parser.add_argument("--output", type=Path, default=HERE / "bikecomp-oled.png")
    parser.add_argument("--scale", type=int, default=6)
    parser.add_argument("--contact-sheet", action="store_true")
    args = parser.parse_args()

    if args.scale < 1:
        parser.error("--scale must be at least 1")
    if args.contact_sheet:
        render_contact_sheet(args.output, args.scale, args.display_height)
    else:
        save_scaled(
            render_raw(args.scenario or "trip", args.display_height),
            args.output,
            args.scale,
        )
    print(args.output)


if __name__ == "__main__":
    main()
