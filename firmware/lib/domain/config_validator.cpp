#include "config_validator.h"

#include <stddef.h>

namespace bike {
namespace {

bool validOptionalRange(uint16_t value, uint16_t minimum, uint16_t maximum) {
  return value == 0 || (value >= minimum && value <= maximum);
}

bool validNameCharacter(char value) {
  return (value >= 'A' && value <= 'Z') ||
         (value >= 'a' && value <= 'z') ||
         (value >= '0' && value <= '9') || value == '-' || value == '_' ||
         value == ' ';
}

}  // namespace

ConfigValidationError ConfigValidator::validate(const DeviceConfig& config) {
  if (config.wheel_circumference_mm < 500 ||
      config.wheel_circumference_mm > 3000) {
    return ConfigValidationError::kWheelCircumference;
  }
  if (config.max_speed_kmh < 20 || config.max_speed_kmh > 200) {
    return ConfigValidationError::kMaxSpeed;
  }
  if (config.stop_timeout_s < 1 || config.stop_timeout_s > 30) {
    return ConfigValidationError::kStopTimeout;
  }
  if (!validOptionalRange(config.display_timeout_s, 10, 600)) {
    return ConfigValidationError::kDisplayTimeout;
  }
  if (!validOptionalRange(config.deep_sleep_timeout_s, 60, 3600)) {
    return ConfigValidationError::kDeepSleepTimeout;
  }
  if (config.brightness_pct < 1 || config.brightness_pct > 100) {
    return ConfigValidationError::kBrightness;
  }
  if (config.page_switch_period_s < 1 || config.page_switch_period_s > 60) {
    return ConfigValidationError::kPageSwitchPeriod;
  }
  if (config.enabled_pages_mask == 0 ||
      (config.enabled_pages_mask & ~kValidEnabledPagesMask) != 0) {
    return ConfigValidationError::kEnabledPagesMask;
  }
  if (config.low_battery_pct < 5 || config.low_battery_pct > 50) {
    return ConfigValidationError::kLowBattery;
  }
  if (config.odometer_save_interval_m < 100 ||
      config.odometer_save_interval_m > 5000) {
    return ConfigValidationError::kOdometerSaveInterval;
  }
  if (config.smoothing_window < 2 || config.smoothing_window > 5) {
    return ConfigValidationError::kSmoothingWindow;
  }
  if (config.debounce_ms > 50) return ConfigValidationError::kDebounce;
  if (config.active_edge > 2) return ConfigValidationError::kActiveEdge;
  if (config.pinned_page >= kDisplayPageCount) {
    return ConfigValidationError::kPinnedPage;
  }
  if (config.batt_cal_scale_permille < 800 ||
      config.batt_cal_scale_permille > 1200) {
    return ConfigValidationError::kBatteryScale;
  }
  if (config.batt_cal_offset_mv < -500 || config.batt_cal_offset_mv > 500) {
    return ConfigValidationError::kBatteryOffset;
  }

  uint8_t seen_pages = 0;
  for (uint8_t page : config.page_order) {
    if (page >= kDisplayPageCount || (seen_pages & (1u << page)) != 0) {
      return ConfigValidationError::kPageOrder;
    }
    seen_pages |= static_cast<uint8_t>(1u << page);
  }

  size_t name_length = 0;
  while (name_length < sizeof(config.device_name) &&
         config.device_name[name_length] != '\0') {
    if (!validNameCharacter(config.device_name[name_length])) {
      return ConfigValidationError::kDeviceName;
    }
    ++name_length;
  }
  if (name_length < 3 || name_length > 15 ||
      name_length == sizeof(config.device_name)) {
    return ConfigValidationError::kDeviceName;
  }
  for (size_t i = name_length + 1; i < sizeof(config.device_name); ++i) {
    if (config.device_name[i] != '\0') {
      return ConfigValidationError::kDeviceName;
    }
  }
  return ConfigValidationError::kNone;
}

}  // namespace bike
