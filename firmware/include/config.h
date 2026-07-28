#pragma once

#include <stdint.h>

namespace bike {

constexpr uint16_t kDefaultWheelCircumferenceMm = 2100;
constexpr uint8_t kDefaultMaxSpeedKmh = 100;
constexpr uint8_t kDefaultDebounceMs = 3;
constexpr uint8_t kDefaultStopTimeoutS = 3;
constexpr uint16_t kDefaultDisplayTimeoutS = 60;
constexpr uint8_t kDefaultSmoothingWindow = 3;
constexpr uint8_t kPulseBufferSize = 8;
constexpr uint8_t kDisplayI2cAddress = 0x3C;

struct DeviceConfig {
  uint16_t wheel_circumference_mm = kDefaultWheelCircumferenceMm;
  uint8_t max_speed_kmh = kDefaultMaxSpeedKmh;
  uint8_t debounce_ms = kDefaultDebounceMs;
  uint8_t stop_timeout_s = kDefaultStopTimeoutS;
  uint16_t display_timeout_s = kDefaultDisplayTimeoutS;
  bool smoothing_enabled = true;
  uint8_t smoothing_window = kDefaultSmoothingWindow;
};

}  // namespace bike
