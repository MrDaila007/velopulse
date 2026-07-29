#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "config.h"
#include "display_formatter.h"
#include "display_power.h"
#include "page_carousel.h"
#include "types.h"

namespace bike {

class DisplayManager {
 public:
  DisplayManager();
  bool begin(const DeviceConfig& config);
  void noteActivity(uint32_t now_ms);
  bool updatePower(uint32_t now_ms);
  void render(const DisplaySnapshot& snapshot, bool force = false);
  bool isOk() const { return display_ok_; }
  DisplayPowerState powerState() const { return power_.state(); }

 private:
  void applyPowerHardware();

  U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display_;
  PageCarousel carousel_;
  DisplayPower power_;
  bool display_ok_ = false;
  uint32_t last_render_ms_ = 0;
};

}  // namespace bike
