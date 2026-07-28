#pragma once

#include <stdint.h>

#include "types.h"

namespace bike {

class BatteryModel {
 public:
  static uint16_t rawToMillivolts(uint16_t raw, uint16_t scale_permille,
                                  int16_t offset_mv);
  static uint8_t voltageToPercent(uint16_t millivolts);

  bool addVoltageSample(uint16_t millivolts);
  BatterySnapshot recalculate(bool usb_present, uint8_t low_battery_pct);
  const BatterySnapshot& snapshot() const { return snapshot_; }

 private:
  BatterySnapshot snapshot_;
  bool filter_initialized_ = false;
  bool percent_initialized_ = false;
};

}  // namespace bike
