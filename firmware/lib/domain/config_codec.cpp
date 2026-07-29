#include "config_codec.h"

#include <string.h>

#include "config_validator.h"

namespace bike {
namespace {

void writeU16(uint8_t* output, uint16_t value) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8u);
}

uint16_t readU16(const uint8_t* input) {
  return static_cast<uint16_t>(input[0]) |
         static_cast<uint16_t>(input[1]) << 8u;
}

}  // namespace

void encodeDeviceConfig(const DeviceConfig& config,
                        uint8_t output[kDeviceConfigPayloadSize]) {
  memset(output, 0, kDeviceConfigPayloadSize);
  output[0] = kDeviceConfigVersion;
  output[1] = (config.smoothing_enabled ? 1u << 0 : 0u) |
              (config.auto_page_switch ? 1u << 1 : 0u) |
              (config.display_auto_off ? 1u << 2 : 0u) |
              (config.ble_always_advertise ? 1u << 3 : 0u) |
              (config.units_imperial ? 1u << 4 : 0u) |
              (config.sensor_invert ? 1u << 5 : 0u) |
              (config.power_save_mode ? 1u << 6 : 0u) |
              (config.deep_sleep_enabled ? 1u << 7 : 0u);
  writeU16(output + 2, config.wheel_circumference_mm);
  output[4] = config.max_speed_kmh;
  output[5] = config.stop_timeout_s;
  writeU16(output + 6, config.display_timeout_s);
  writeU16(output + 8, config.deep_sleep_timeout_s);
  output[10] = config.brightness_pct;
  output[11] = config.page_switch_period_s;
  output[12] = config.enabled_pages_mask;
  output[13] = config.low_battery_pct;
  writeU16(output + 14, config.odometer_save_interval_m);
  output[16] = config.smoothing_window;
  output[17] = config.debounce_ms;
  output[18] = config.active_edge;
  output[19] = config.pinned_page;
  writeU16(output + 20, config.batt_cal_scale_permille);
  writeU16(output + 22, static_cast<uint16_t>(config.batt_cal_offset_mv));
  memcpy(output + 24, config.page_order, kDisplayPageCount);
  for (size_t i = 0; i < 15 && config.device_name[i] != '\0'; ++i) {
    output[30 + i] = static_cast<uint8_t>(config.device_name[i]);
  }
}

bool decodeDeviceConfig(const uint8_t* input,
                        size_t length,
                        DeviceConfig& config) {
  if (input == nullptr || length != kDeviceConfigPayloadSize ||
      input[0] != kDeviceConfigVersion || input[29] != 0 ||
      input[46] != 0 || input[47] != 0) {
    return false;
  }

  DeviceConfig decoded;
  const uint8_t flags = input[1];
  decoded.smoothing_enabled = (flags & (1u << 0)) != 0;
  decoded.auto_page_switch = (flags & (1u << 1)) != 0;
  decoded.display_auto_off = (flags & (1u << 2)) != 0;
  decoded.ble_always_advertise = (flags & (1u << 3)) != 0;
  decoded.units_imperial = (flags & (1u << 4)) != 0;
  decoded.sensor_invert = (flags & (1u << 5)) != 0;
  decoded.power_save_mode = (flags & (1u << 6)) != 0;
  decoded.deep_sleep_enabled = (flags & (1u << 7)) != 0;
  decoded.wheel_circumference_mm = readU16(input + 2);
  decoded.max_speed_kmh = input[4];
  decoded.stop_timeout_s = input[5];
  decoded.display_timeout_s = readU16(input + 6);
  decoded.deep_sleep_timeout_s = readU16(input + 8);
  decoded.brightness_pct = input[10];
  decoded.page_switch_period_s = input[11];
  decoded.enabled_pages_mask = input[12];
  decoded.low_battery_pct = input[13];
  decoded.odometer_save_interval_m = readU16(input + 14);
  decoded.smoothing_window = input[16];
  decoded.debounce_ms = input[17];
  decoded.active_edge = input[18];
  decoded.pinned_page = input[19];
  decoded.batt_cal_scale_permille = readU16(input + 20);
  decoded.batt_cal_offset_mv = static_cast<int16_t>(readU16(input + 22));
  memcpy(decoded.page_order, input + 24, kDisplayPageCount);
  memcpy(decoded.device_name, input + 30, sizeof(decoded.device_name));

  if (ConfigValidator::validate(decoded) != ConfigValidationError::kNone) {
    return false;
  }
  config = decoded;
  return true;
}

bool deviceConfigsEqual(const DeviceConfig& left, const DeviceConfig& right) {
  uint8_t left_bytes[kDeviceConfigPayloadSize];
  uint8_t right_bytes[kDeviceConfigPayloadSize];
  encodeDeviceConfig(left, left_bytes);
  encodeDeviceConfig(right, right_bytes);
  return memcmp(left_bytes, right_bytes, kDeviceConfigPayloadSize) == 0;
}

}  // namespace bike
