#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "config.h"
#include "display_burn_in.h"
#include "display_formatter.h"
#include "display_profile.h"
#include "display_power.h"
#include "page_carousel.h"
#include "types.h"

namespace bike {

#if BIKECOMP_DISPLAY_HEIGHT == 64
using DisplayDriver = U8G2_SSD1306_128X64_NONAME_F_HW_I2C;
#else
using DisplayDriver = U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C;
#endif

class DisplayManager {
 public:
  DisplayManager();
  bool begin(const DeviceConfig& config);
  void applyRuntimeConfig(const DeviceConfig& config, uint32_t now_ms);
  void noteActivity(uint32_t now_ms);
  void turnOff(uint32_t now_ms);
  void showTestPattern(uint8_t pattern, uint32_t now_ms);
  bool updatePower(uint32_t now_ms);
  void setAmbientBrightness(uint8_t brightness_pct, bool valid);
  void render(const DisplaySnapshot& snapshot, bool force = false);
  bool isOk() const { return display_ok_; }
  bool displayTestActive() const { return test_active_; }
  DisplayPowerState powerState() const { return power_.state(); }
  uint8_t effectiveBrightnessPct() const { return effective_brightness_pct_; }

 private:
  void applyPowerHardware();
  void updateEffectiveBrightness();

  DisplayDriver display_;
  PageCarousel carousel_;
  DisplayPower power_;
  DisplayBurnInGuard burn_in_;
  bool display_ok_ = false;
  uint32_t last_render_ms_ = 0;
  uint32_t test_started_ms_ = 0;
  bool test_active_ = false;
  uint8_t manual_brightness_pct_ = kDefaultBrightnessPct;
  uint8_t ambient_brightness_pct_ = 100;
  uint8_t effective_brightness_pct_ = kDefaultBrightnessPct;
  uint8_t bright_contrast_ = 156;
  bool ambient_valid_ = false;
};

}  // namespace bike
