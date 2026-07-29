#pragma once

#include <stdint.h>

namespace bike {

constexpr uint16_t kDefaultWheelCircumferenceMm = 2100;
constexpr uint8_t kDefaultMaxSpeedKmh = 100;
constexpr uint8_t kDefaultDebounceMs = 3;
constexpr uint8_t kDefaultStopTimeoutS = 3;
constexpr uint16_t kDefaultDisplayTimeoutS = 60;
constexpr uint16_t kDefaultDeepSleepTimeoutS = 900;
constexpr uint8_t kDefaultBrightnessPct = 60;
constexpr uint8_t kDefaultSmoothingWindow = 3;
constexpr uint8_t kDisplayPageCount = 5;
constexpr uint8_t kDefaultEnabledPagesMask = 0x1F;
constexpr uint8_t kDefaultPageSwitchPeriodS = 4;
constexpr uint8_t kDefaultLowBatteryPct = 20;
constexpr uint16_t kDefaultOdometerSaveIntervalM = 500;
constexpr uint16_t kDefaultBatteryCalScalePermille = 1000;
constexpr int16_t kDefaultBatteryCalOffsetMv = 0;
constexpr char kDefaultDeviceName[] = "BikeComp-XXXX";
constexpr uint8_t kPulseBufferSize = 8;
constexpr uint8_t kDisplayI2cAddress = 0x3C;

struct DeviceConfig {
  uint16_t wheel_circumference_mm = kDefaultWheelCircumferenceMm;
  uint8_t max_speed_kmh = kDefaultMaxSpeedKmh;
  uint8_t stop_timeout_s = kDefaultStopTimeoutS;
  uint16_t display_timeout_s = kDefaultDisplayTimeoutS;
  uint16_t deep_sleep_timeout_s = kDefaultDeepSleepTimeoutS;
  uint8_t brightness_pct = kDefaultBrightnessPct;
  uint8_t page_switch_period_s = kDefaultPageSwitchPeriodS;
  uint8_t enabled_pages_mask = kDefaultEnabledPagesMask;
  uint8_t low_battery_pct = kDefaultLowBatteryPct;
  uint16_t odometer_save_interval_m = kDefaultOdometerSaveIntervalM;
  uint8_t smoothing_window = kDefaultSmoothingWindow;
  uint8_t debounce_ms = kDefaultDebounceMs;
  uint8_t active_edge = 0;
  uint8_t pinned_page = 0;
  uint16_t batt_cal_scale_permille = kDefaultBatteryCalScalePermille;
  int16_t batt_cal_offset_mv = kDefaultBatteryCalOffsetMv;
  uint8_t page_order[kDisplayPageCount] = {0, 1, 2, 3, 4};
  char device_name[16] = "BikeComp-XXXX";

  bool smoothing_enabled = true;
  bool auto_page_switch = true;
  bool display_auto_off = true;
  bool ble_always_advertise = true;
  bool units_imperial = false;
  bool sensor_invert = false;
  bool power_save_mode = false;
  bool deep_sleep_enabled = false;
};

}  // namespace bike
