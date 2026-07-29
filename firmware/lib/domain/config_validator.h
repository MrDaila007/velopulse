#pragma once

#include <stdint.h>

#include "config.h"

namespace bike {

enum class ConfigValidationError : uint8_t {
  kNone = 0,
  kWheelCircumference,
  kMaxSpeed,
  kStopTimeout,
  kDisplayTimeout,
  kDeepSleepTimeout,
  kBrightness,
  kPageSwitchPeriod,
  kEnabledPagesMask,
  kLowBattery,
  kOdometerSaveInterval,
  kSmoothingWindow,
  kDebounce,
  kActiveEdge,
  kPinnedPage,
  kBatteryScale,
  kBatteryOffset,
  kPageOrder,
  kDeviceName,
};

class ConfigValidator {
 public:
  static ConfigValidationError validate(const DeviceConfig& config);
};

}  // namespace bike
