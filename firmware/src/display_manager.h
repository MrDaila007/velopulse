#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "types.h"

namespace bike {

class DisplayManager {
 public:
  DisplayManager();
  bool begin();
  void render(const TripSnapshot& snapshot, bool force = false);
  bool isOk() const { return display_ok_; }

 private:
  U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display_;
  bool display_ok_ = false;
  uint32_t last_render_ms_ = 0;
};

}  // namespace bike
