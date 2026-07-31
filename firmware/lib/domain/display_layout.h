#pragma once

#include <stdint.h>

#include "types.h"

namespace bike {

enum class DisplayProfile : uint8_t { k128x32, k128x64 };

enum class DisplayFont : uint8_t { kSpeed, kSmall, kSpeedLarge, kMetricLarge };

class DisplayCanvas {
 public:
  virtual ~DisplayCanvas() = default;
  virtual void setFont(DisplayFont font) = 0;
  virtual void setDrawColor(uint8_t color) = 0;
  virtual void drawText(int16_t x, int16_t y, const char* text) = 0;
  virtual void drawFrame(int16_t x, int16_t y, uint8_t width, uint8_t height) = 0;
  virtual void drawBox(int16_t x, int16_t y, uint8_t width, uint8_t height) = 0;
};

void drawDisplayFrame(DisplayCanvas& canvas, const DisplayFrame& frame,
                      DisplayProfile profile, int8_t x_offset = 0,
                      int8_t y_offset = 0);

}  // namespace bike
