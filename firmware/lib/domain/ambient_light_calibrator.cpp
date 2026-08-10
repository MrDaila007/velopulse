#include "ambient_light_calibrator.h"

namespace bike {

void AmbientLightCalibrator::configure(uint16_t raw_dark, uint16_t raw_bright) {
  raw_dark_ = raw_dark;
  raw_bright_ = raw_bright;
  changed_ = false;
}

void AmbientLightCalibrator::addSample(uint16_t raw) {
  if (raw <= kAmbientInvalidRailMargin ||
      raw >= kAmbientAdcMaximum - kAmbientInvalidRailMargin) {
    return;
  }
  if (raw < raw_dark_) {
    raw_dark_ = raw;
    changed_ = true;
  }
  if (raw > raw_bright_) {
    raw_bright_ = raw;
    changed_ = true;
  }
}

AmbientCalibrationQuality AmbientLightCalibrator::quality() const {
  if (raw_bright_ <= raw_dark_) return AmbientCalibrationQuality::kNarrow;
  const uint16_t width = static_cast<uint16_t>(raw_bright_ - raw_dark_);
  if (width < kAmbientCalibrationMinWidth) return AmbientCalibrationQuality::kNarrow;
  if (raw_dark_ > kAmbientCalibrationMaxAcceptableDark) {
    return AmbientCalibrationQuality::kNarrow;
  }
  if (raw_bright_ < kAmbientCalibrationMinAcceptableBright) {
    return AmbientCalibrationQuality::kNarrow;
  }
  return AmbientCalibrationQuality::kOk;
}

const char* ambientCalibrationQualityName(AmbientCalibrationQuality quality) {
  return quality == AmbientCalibrationQuality::kOk ? "ok" : "narrow";
}

}  // namespace bike
