#include "battery_manager.h"

#include "board_pins.h"

namespace bike {
namespace {

constexpr uint32_t kSamplePeriodMs = 1000;
constexpr uint32_t kPercentPeriodMs = 10000;
constexpr uint8_t kSampleCount = 16;
constexpr uint16_t kDisplayLoadCompensationMv = 25;

}  // namespace

void BatteryManager::begin(const DeviceConfig& config, uint32_t now_ms) {
  config_ = &config;
  analogReference(AR_INTERNAL_2_4);
  analogReadResolution(12);
  analogSampleTime(40);
  pinMode(kBatteryAdcPin, INPUT);
  sample();
  model_.recalculate(usbPresent(), config.low_battery_pct);
  last_sample_ms_ = now_ms;
  last_percent_ms_ = now_ms;
}

void BatteryManager::applyRuntimeConfig(const DeviceConfig& config,
                                        uint32_t now_ms) {
  config_ = &config;
  model_.recalculate(usbPresent(), config.low_battery_pct);
  last_percent_ms_ = now_ms;
  (void)now_ms;
}

bool BatteryManager::update(uint32_t now_ms) {
  if (config_ == nullptr) return false;
  if (static_cast<uint32_t>(now_ms - last_sample_ms_) >= kSamplePeriodMs) {
    sample();
    last_sample_ms_ = now_ms;
  }
  if (static_cast<uint32_t>(now_ms - last_percent_ms_) < kPercentPeriodMs) {
    return false;
  }
  model_.recalculate(usbPresent(), config_->low_battery_pct);
  last_percent_ms_ = now_ms;
  return true;
}

bool BatteryManager::sample() {
  if (config_ == nullptr) return false;

  uint16_t minimum = 4095;
  uint16_t maximum = 0;
  uint32_t sum = 0;
  for (uint8_t i = 0; i < kSampleCount; ++i) {
    const uint16_t raw = static_cast<uint16_t>(analogRead(kBatteryAdcPin));
    if (raw < minimum) minimum = raw;
    if (raw > maximum) maximum = raw;
    sum += raw;
  }

  last_raw_spread_ = maximum - minimum;
  last_raw_average_ =
      static_cast<uint16_t>((sum - minimum - maximum + 7u) / 14u);
  uint16_t millivolts = BatteryModel::rawToMillivolts(
      last_raw_average_, config_->batt_cal_scale_permille,
      config_->batt_cal_offset_mv);
  if (millivolts <= 65535u - kDisplayLoadCompensationMv) {
    millivolts += kDisplayLoadCompensationMv;
  }
  return model_.addVoltageSample(millivolts);
}

bool BatteryManager::usbPresent() const {
  return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0;
}

}  // namespace bike
