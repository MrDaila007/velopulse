#pragma once

#include <stdint.h>

namespace bike {

constexpr uint16_t kAmbientAdcMaximum = 4095u;
constexpr uint32_t kAmbientMinimumLevelDwellMs = 2000u;

struct AmbientLightSnapshot {
  uint16_t raw = 0;
  uint16_t filtered_raw = 0;
  uint16_t normalized_permille = 0;
  uint8_t brightness_pct = 100;
  bool valid = false;
};

class AmbientLightModel {
 public:
  void configure(uint16_t raw_dark, uint16_t raw_bright, uint32_t now_ms);
  bool addSample(uint16_t raw, uint32_t now_ms);

  const AmbientLightSnapshot& snapshot() const { return snapshot_; }
  uint8_t cappedBrightness(uint8_t maximum_pct) const;

  static uint16_t normalize(uint16_t raw, uint16_t raw_dark,
                            uint16_t raw_bright);
  static uint8_t brightnessForNormalized(uint16_t normalized_permille);
  static uint8_t contrastForBrightness(uint8_t brightness_pct);

 private:
  static uint8_t levelForNormalized(uint16_t normalized_permille);
  bool canChangeLevel(uint8_t candidate_level,
                      uint16_t normalized_permille) const;

  AmbientLightSnapshot snapshot_;
  uint16_t raw_dark_ = 0;
  uint16_t raw_bright_ = kAmbientAdcMaximum;
  uint32_t last_level_change_ms_ = 0;
  uint8_t level_ = 4;
  bool filter_initialized_ = false;
};

}  // namespace bike
