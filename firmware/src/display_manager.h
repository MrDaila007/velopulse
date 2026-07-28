#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "config.h"
#include "display_formatter.h"
#include "page_carousel.h"
#include "types.h"

namespace bike {

class DisplayManager {
 public:
  DisplayManager();
  bool begin(const DeviceConfig& config);
  void render(const DisplaySnapshot& snapshot, bool force = false);
  bool isOk() const { return display_ok_; }

 private:
  U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display_;
  PageCarousel carousel_;
  bool display_ok_ = false;
  uint32_t last_render_ms_ = 0;
};

}  // namespace bike
