#pragma once

#include <stdint.h>

#include "ambient_light_model.h"

namespace bike {

constexpr uint16_t kAmbientCalibrationMinWidth = 400u;
constexpr uint16_t kAmbientCalibrationMaxAcceptableDark = 600u;
constexpr uint16_t kAmbientCalibrationMinAcceptableBright = 900u;

enum class AmbientCalibrationQuality : uint8_t {
  kNarrow = 0,
  kOk = 1,
};

class AmbientLightCalibrator {
 public:
  void configure(uint16_t raw_dark, uint16_t raw_bright);
  void addSample(uint16_t raw);

  uint16_t rawDark() const { return raw_dark_; }
  uint16_t rawBright() const { return raw_bright_; }
  AmbientCalibrationQuality quality() const;
  bool changed() const { return changed_; }
  void markPersisted() { changed_ = false; }

 private:
  uint16_t raw_dark_ = 0;
  uint16_t raw_bright_ = kAmbientAdcMaximum;
  bool changed_ = false;
};

const char* ambientCalibrationQualityName(AmbientCalibrationQuality quality);

}  // namespace bike
