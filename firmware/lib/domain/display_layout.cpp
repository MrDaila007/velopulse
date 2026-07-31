#include "display_layout.h"

#include <string.h>

namespace bike {
namespace {

class OffsetCanvas final : public DisplayCanvas {
 public:
  OffsetCanvas(DisplayCanvas& canvas, int8_t x_offset, int8_t y_offset)
      : canvas_(canvas), x_offset_(x_offset), y_offset_(y_offset) {}

  void setFont(DisplayFont font) override { canvas_.setFont(font); }
  void setDrawColor(uint8_t color) override { canvas_.setDrawColor(color); }
  void drawText(int16_t x, int16_t y, const char* text) override {
    canvas_.drawText(x + x_offset_, y + y_offset_, text);
  }
  void drawFrame(int16_t x, int16_t y, uint8_t width,
                 uint8_t height) override {
    canvas_.drawFrame(x + x_offset_, y + y_offset_, width, height);
  }
  void drawBox(int16_t x, int16_t y, uint8_t width,
               uint8_t height) override {
    canvas_.drawBox(x + x_offset_, y + y_offset_, width, height);
  }

 private:
  DisplayCanvas& canvas_;
  int8_t x_offset_;
  int8_t y_offset_;
};

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
                      DisplayProfile profile, int8_t x_offset,
                      int8_t y_offset) {
  OffsetCanvas offset_canvas(canvas, x_offset, y_offset);
  if (profile == DisplayProfile::k128x64) {
    draw128x64(offset_canvas, frame);
  } else {
    draw128x32(offset_canvas, frame);
  }
}

}  // namespace bike
