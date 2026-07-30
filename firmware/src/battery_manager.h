#pragma once

#include <Arduino.h>

#include "battery_model.h"
#include "config.h"
#include "types.h"

namespace bike {

class BatteryManager {
 public:
  void begin(const DeviceConfig& config, uint32_t now_ms);
  void applyRuntimeConfig(const DeviceConfig& config, uint32_t now_ms);
  bool update(uint32_t now_ms);

  const BatterySnapshot& snapshot() const { return model_.snapshot(); }
  uint16_t lastRawAverage() const { return last_raw_average_; }
  uint16_t lastRawSpread() const { return last_raw_spread_; }

 private:
  bool sample();
  bool usbPresent() const;

  BatteryModel model_;
  const DeviceConfig* config_ = nullptr;
  uint32_t last_sample_ms_ = 0;
  uint32_t last_percent_ms_ = 0;
  uint16_t last_raw_average_ = 0;
  uint16_t last_raw_spread_ = 0;
};

}  // namespace bike
