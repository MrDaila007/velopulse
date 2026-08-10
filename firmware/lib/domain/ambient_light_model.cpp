#include "ambient_light_model.h"

namespace bike {
namespace {

constexpr uint16_t kLevelThresholds[] = {150u, 350u, 600u, 800u};
constexpr uint8_t kBrightnessLevels[] = {5u, 15u, 35u, 65u, 100u};
constexpr uint16_t kHysteresisPermille = 50u;

}  // namespace

void AmbientLightModel::configure(uint16_t raw_dark, uint16_t raw_bright,
                                  uint32_t now_ms) {
  raw_dark_ = raw_dark;
  raw_bright_ = raw_bright;
  snapshot_ = AmbientLightSnapshot{};
  last_level_change_ms_ = now_ms;
  level_ = 4;
  filter_initialized_ = false;
}

uint16_t AmbientLightModel::normalize(uint16_t raw, uint16_t raw_dark,
                                      uint16_t raw_bright) {
  if (raw_bright <= raw_dark) return 0;
  if (raw <= raw_dark) return 0;
  if (raw >= raw_bright) return 1000u;
  return static_cast<uint16_t>(
      (static_cast<uint32_t>(raw - raw_dark) * 1000u +
       (raw_bright - raw_dark) / 2u) /
      (raw_bright - raw_dark));
}

uint8_t AmbientLightModel::levelForNormalized(uint16_t normalized_permille) {
  for (uint8_t i = 0; i < 4u; ++i) {
    if (normalized_permille < kLevelThresholds[i]) return i;
  }
  return 4u;
}

uint8_t AmbientLightModel::brightnessForNormalized(
    uint16_t normalized_permille) {
  return kBrightnessLevels[levelForNormalized(normalized_permille)];
}

uint8_t AmbientLightModel::contrastForBrightness(uint8_t brightness_pct) {
  if (brightness_pct < 1u) brightness_pct = 1u;
  if (brightness_pct > 100u) brightness_pct = 100u;
  return static_cast<uint8_t>(
      8u + (static_cast<uint16_t>(brightness_pct) * 247u) / 100u);
}

bool AmbientLightModel::canChangeLevel(
    uint8_t candidate_level, uint16_t normalized_permille) const {
  if (candidate_level == level_) return false;
  if (candidate_level > level_) {
    const uint16_t boundary = kLevelThresholds[level_];
    return normalized_permille >= boundary + kHysteresisPermille;
  }
  const uint16_t boundary = kLevelThresholds[level_ - 1u];
  return normalized_permille + kHysteresisPermille < boundary;
}

bool AmbientLightModel::addSample(uint16_t raw, uint32_t now_ms) {
  const bool was_valid = snapshot_.valid;
  const uint8_t previous_brightness = snapshot_.brightness_pct;
  snapshot_.raw = raw;

  if (raw <= kAmbientInvalidRailMargin ||
      raw >= kAmbientAdcMaximum - kAmbientInvalidRailMargin ||
      raw_bright_ <= raw_dark_) {
    snapshot_.valid = false;
    snapshot_.brightness_pct = 100u;
    filter_initialized_ = false;
    return was_valid || previous_brightness != snapshot_.brightness_pct;
  }

  if (!filter_initialized_) {
    snapshot_.filtered_raw = raw;
    filter_initialized_ = true;
    snapshot_.normalized_permille = normalize(raw, raw_dark_, raw_bright_);
    level_ = levelForNormalized(snapshot_.normalized_permille);
    snapshot_.brightness_pct = kBrightnessLevels[level_];
    snapshot_.valid = true;
    last_level_change_ms_ = now_ms;
    return !was_valid || previous_brightness != snapshot_.brightness_pct;
  }

  snapshot_.filtered_raw = static_cast<uint16_t>(
      (static_cast<uint32_t>(snapshot_.filtered_raw) * 7u + raw + 4u) / 8u);
  snapshot_.normalized_permille =
      normalize(snapshot_.filtered_raw, raw_dark_, raw_bright_);
  snapshot_.valid = true;

  const uint8_t candidate = levelForNormalized(snapshot_.normalized_permille);
  if (static_cast<uint32_t>(now_ms - last_level_change_ms_) >=
          kAmbientMinimumLevelDwellMs &&
      canChangeLevel(candidate, snapshot_.normalized_permille)) {
    level_ = candidate;
    snapshot_.brightness_pct = kBrightnessLevels[level_];
    last_level_change_ms_ = now_ms;
  }
  return !was_valid || previous_brightness != snapshot_.brightness_pct;
}

uint8_t AmbientLightModel::cappedBrightness(uint8_t maximum_pct) const {
  if (maximum_pct < 1u) maximum_pct = 1u;
  if (maximum_pct > 100u) maximum_pct = 100u;
  if (!snapshot_.valid || snapshot_.brightness_pct > maximum_pct) {
    return maximum_pct;
  }
  return snapshot_.brightness_pct;
}

}  // namespace bike
