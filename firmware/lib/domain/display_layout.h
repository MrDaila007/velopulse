#pragma once

#include <stdint.h>

#include "types.h"

namespace bike {

enum class DisplayFont : uint8_t { kSpeed, kSmall };

class DisplayCanvas {
 public:
  virtual ~DisplayCanvas() = default;
  virtual void setFont(DisplayFont font) = 0;
  virtual void setDrawColor(uint8_t color) = 0;
  virtual void drawText(int16_t x, int16_t y, const char* text) = 0;
  virtual void drawFrame(int16_t x, int16_t y, uint8_t width, uint8_t height) = 0;
  virtual void drawBox(int16_t x, int16_t y, uint8_t width, uint8_t height) = 0;
};

void drawDisplayFrame(DisplayCanvas& canvas, const DisplayFrame& frame);

}  // namespace bike
