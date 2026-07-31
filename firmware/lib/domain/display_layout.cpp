#include "display_layout.h"

#include <string.h>

namespace bike {
namespace {

void draw128x32(DisplayCanvas& canvas, const DisplayFrame& frame) {
  canvas.setFont(DisplayFont::kSpeed);
  canvas.drawText(0, 21, frame.speed);

  canvas.setFont(DisplayFont::kSmall);
  canvas.drawFrame(91, 0, 12, 8);
  canvas.drawBox(103, 2, 2, 4);
  if (frame.battery_fill_width != 0) {
    canvas.drawBox(93, 2, frame.battery_fill_width, 4);
  }
  canvas.drawText(107, 7, frame.battery_percent);
  canvas.drawText(88, 20, frame.units);
  if (frame.low_battery_warning) {
    canvas.drawBox(0, 22, 44, 10);
    canvas.setDrawColor(0);
    canvas.drawText(2, 30, frame.lower);
    canvas.setDrawColor(1);
  } else {
    canvas.drawText(0, 31, frame.lower);
  }
}

void drawBattery128x64(DisplayCanvas& canvas, const DisplayFrame& frame) {
  canvas.drawFrame(91, 0, 12, 8);
  canvas.drawBox(103, 2, 2, 4);
  if (frame.battery_fill_width != 0) {
    canvas.drawBox(93, 2, frame.battery_fill_width, 4);
  }
  canvas.drawText(107, 7, frame.battery_percent);
}

void draw128x64(DisplayCanvas& canvas, const DisplayFrame& frame) {
  canvas.setFont(DisplayFont::kSmall);
  canvas.drawText(0, 7, frame.units);
  drawBattery128x64(canvas, frame);

  canvas.setFont(DisplayFont::kSpeedLarge);
  canvas.drawText(0, 47, frame.speed);

  const bool use_large_metric = strlen(frame.lower) <= 21u;
  canvas.setFont(use_large_metric ? DisplayFont::kMetricLarge
                                  : DisplayFont::kSmall);
  if (frame.low_battery_warning) {
    canvas.drawBox(0, 50, 128, 14);
    canvas.setDrawColor(0);
    canvas.drawText(2, 63, frame.lower);
    canvas.setDrawColor(1);
  } else {
    canvas.drawText(0, 63, frame.lower);
  }
}

}  // namespace

void drawDisplayFrame(DisplayCanvas& canvas, const DisplayFrame& frame,
                      DisplayProfile profile) {
  if (profile == DisplayProfile::k128x64) {
    draw128x64(canvas, frame);
  } else {
    draw128x32(canvas, frame);
  }
}

}  // namespace bike
