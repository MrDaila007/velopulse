#pragma once

#include <stdint.h>

#include "display_burn_in.h"
#include "display_formatter.h"
#include "display_power.h"
#include "page_carousel.h"
#include "types.h"
#include "config.h"

namespace bike {

class DisplayManager {
 public:
  DisplayManager() = default;
  bool begin(const DeviceConfig& config);
  void applyRuntimeConfig(const DeviceConfig& config, uint32_t now_ms);
  void noteActivity(uint32_t now_ms);
  void turnOff(uint32_t now_ms);
  void showTestPattern(uint8_t pattern, uint32_t now_ms);
  bool updatePower(uint32_t now_ms);
  void setAmbientBrightness(uint8_t brightness_pct, bool valid);
  void render(const DisplaySnapshot& snapshot, bool force = false);
  bool isOk() const { return display_ok_; }
  DisplayPowerState powerState() const { return power_.state(); }
  uint8_t effectiveBrightnessPct() const { return effective_brightness_pct_; }

 private:
  void updateEffectiveBrightness();

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
  bool ambient_valid_ = false;
};

}  // namespace bike
