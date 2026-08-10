#include "display_layout.h"

#include <string.h>

namespace bike {
namespace {

constexpr int16_t kRightColumnX = 91;
constexpr int16_t kBatteryIconFrameX = 107;
constexpr int16_t kBatteryIconFillX = 109;
constexpr int16_t kBatteryIconTipX = 119;
constexpr int16_t kBleIndicatorX = 56;
constexpr int16_t kBleIndicatorY = 7;

struct RightColumnLayout {
  int16_t battery_percent_y;
  int16_t battery_voltage_y;
  int16_t clock_y;
  int16_t weather_temp_y;
  int16_t weather_rain_y;
};

// u8g2_font_5x8_tf is 8 px tall; keep at least 8 px between baselines (+2 px gap).
constexpr RightColumnLayout kRightColumn128x32 = {7, 15, 23, 31, 39};
constexpr RightColumnLayout kRightColumn128x64 = {7, 17, 27, 37, 47};

void drawBatteryIcon(DisplayCanvas& canvas, uint8_t fill_width) {
  canvas.drawFrame(kBatteryIconFrameX, 0, 12, 8);
  canvas.drawBox(kBatteryIconTipX, 2, 2, 4);
  if (fill_width != 0) {
    canvas.drawBox(kBatteryIconFillX, 2, fill_width, 4);
  }
}

void drawBatteryLabels(DisplayCanvas& canvas, const DisplayFrame& frame,
                       const RightColumnLayout& layout) {
  canvas.drawText(kRightColumnX, layout.battery_percent_y, frame.battery_percent);
  canvas.drawText(kRightColumnX, layout.battery_voltage_y, frame.battery_voltage);
  if (frame.header[0] != '\0') {
    canvas.drawText(kRightColumnX, layout.clock_y, frame.header);
  }
  if (frame.weather_temp[0] != '\0') {
    canvas.drawText(kRightColumnX, layout.weather_temp_y, frame.weather_temp);
  }
  if (frame.weather_rain[0] != '\0') {
    canvas.drawText(kRightColumnX, layout.weather_rain_y, frame.weather_rain);
  }
}

void drawBleIndicator(DisplayCanvas& canvas, const DisplayFrame& frame) {
  if (!frame.ble_connected) return;
  canvas.drawText(kBleIndicatorX, kBleIndicatorY, "BLE");
}

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
  drawBleIndicator(canvas, frame);
  drawBatteryLabels(canvas, frame, kRightColumn128x32);
  drawBatteryIcon(canvas, frame.battery_fill_width);
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
  drawBatteryLabels(canvas, frame, kRightColumn128x64);
  drawBatteryIcon(canvas, frame.battery_fill_width);
}

void draw128x64(DisplayCanvas& canvas, const DisplayFrame& frame) {
  canvas.setFont(DisplayFont::kSmall);
  canvas.drawText(0, 7, frame.units);
  drawBleIndicator(canvas, frame);
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
