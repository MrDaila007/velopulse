#include "battery_model.h"

namespace bike {
namespace {

constexpr uint16_t kMinimumValidBatteryMv = 2500;
constexpr uint16_t kMaximumValidBatteryMv = 4350;
constexpr uint16_t kDividerNumerator = 1510;
constexpr uint16_t kDividerDenominator = 510;
constexpr uint16_t kAdcReferenceMv = 2400;
constexpr uint16_t kAdcMaximum = 4095;

constexpr uint16_t kVoltageTable[] = {
    3300, 3400, 3500, 3600, 3650, 3700, 3800, 3900, 4000, 4100, 4200,
};
constexpr uint8_t kPercentTable[] = {
    0, 4, 10, 20, 28, 38, 52, 68, 82, 92, 100,
};
constexpr uint8_t kTableSize = sizeof(kVoltageTable) / sizeof(kVoltageTable[0]);

}  // namespace

uint16_t BatteryModel::rawToMillivolts(uint16_t raw, uint16_t scale_permille,
                                       int16_t offset_mv) {
  if (raw > kAdcMaximum) raw = kAdcMaximum;
  const uint64_t numerator = static_cast<uint64_t>(raw) * kAdcReferenceMv *
                             kDividerNumerator;
  const uint32_t denominator =
      static_cast<uint32_t>(kAdcMaximum) * kDividerDenominator;
  const uint32_t nominal_mv =
      static_cast<uint32_t>((numerator + denominator / 2u) / denominator);
  int32_t calibrated_mv =
      static_cast<int32_t>((static_cast<uint64_t>(nominal_mv) * scale_permille +
                            500u) /
                           1000u) +
      offset_mv;
  if (calibrated_mv < 0) calibrated_mv = 0;
  if (calibrated_mv > 65535) calibrated_mv = 65535;
  return static_cast<uint16_t>(calibrated_mv);
}

uint8_t BatteryModel::voltageToPercent(uint16_t millivolts) {
  if (millivolts <= kVoltageTable[0]) return kPercentTable[0];
  if (millivolts >= kVoltageTable[kTableSize - 1]) {
    return kPercentTable[kTableSize - 1];
  }
  for (uint8_t i = 1; i < kTableSize; ++i) {
    if (millivolts <= kVoltageTable[i]) {
      const uint16_t voltage_span = kVoltageTable[i] - kVoltageTable[i - 1];
      const uint8_t percent_span = kPercentTable[i] - kPercentTable[i - 1];
      const uint16_t voltage_offset = millivolts - kVoltageTable[i - 1];
      return static_cast<uint8_t>(
          kPercentTable[i - 1] +
          (static_cast<uint32_t>(voltage_offset) * percent_span +
           voltage_span / 2u) /
              voltage_span);
    }
  }
  return 0;
}

bool BatteryModel::addVoltageSample(uint16_t millivolts) {
  if (millivolts < kMinimumValidBatteryMv ||
      millivolts > kMaximumValidBatteryMv) {
    return false;
  }
  if (!filter_initialized_) {
    snapshot_.millivolts = millivolts;
    filter_initialized_ = true;
  } else {
    const int32_t difference =
        static_cast<int32_t>(millivolts) - snapshot_.millivolts;
    snapshot_.millivolts = static_cast<uint16_t>(
        static_cast<int32_t>(snapshot_.millivolts) + difference / 8);
  }
  snapshot_.valid = true;
  return true;
}

BatterySnapshot BatteryModel::recalculate(bool usb_present,
                                          uint8_t low_battery_pct) {
  snapshot_.usb_present = usb_present;
  snapshot_.charge_status = ChargeStatus::kUnknown;
  if (!filter_initialized_) return snapshot_;

  const uint8_t candidate = voltageToPercent(snapshot_.millivolts);
  if (!percent_initialized_ || usb_present || candidate < snapshot_.percent) {
    snapshot_.percent = candidate;
    percent_initialized_ = true;
  }

  if (!snapshot_.low_battery && snapshot_.percent <= low_battery_pct) {
    snapshot_.low_battery = true;
  } else {
    const uint16_t release_threshold =
        static_cast<uint16_t>(low_battery_pct) + 3u;
    if (snapshot_.low_battery && snapshot_.percent >= release_threshold) {
      snapshot_.low_battery = false;
    }
  }
  return snapshot_;
}

}  // namespace bike
