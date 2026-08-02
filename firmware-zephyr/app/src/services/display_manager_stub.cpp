#include "display_manager.h"

#include "platform.h"

namespace bike {

bool DisplayManager::begin(const DeviceConfig& config) {
  const uint32_t now_ms = platform::millis();
  manual_brightness_pct_ = config.brightness_pct;
  carousel_.configure(config, now_ms);
  power_.configure(config.display_auto_off ? config.display_timeout_s : 0u,
                   now_ms);
  burn_in_.configure(now_ms);
  display_ok_ = false;
  updateEffectiveBrightness();
  return false;
}

void DisplayManager::applyRuntimeConfig(const DeviceConfig& config,
                                        uint32_t now_ms) {
  manual_brightness_pct_ = config.brightness_pct;
  carousel_.configure(config, now_ms);
  power_.configure(config.display_auto_off ? config.display_timeout_s : 0u,
                   now_ms);
  updateEffectiveBrightness();
}

void DisplayManager::noteActivity(uint32_t now_ms) {
  power_.noteActivity(now_ms);
}

void DisplayManager::turnOff(uint32_t now_ms) { power_.forceOff(now_ms); }

void DisplayManager::showTestPattern(uint8_t pattern, uint32_t now_ms) {
  test_active_ = true;
  test_started_ms_ = now_ms;
  (void)pattern;
}

bool DisplayManager::updatePower(uint32_t now_ms) {
  return power_.update(now_ms);
}

void DisplayManager::setAmbientBrightness(uint8_t brightness_pct, bool valid) {
  ambient_brightness_pct_ = brightness_pct;
  ambient_valid_ = valid;
  updateEffectiveBrightness();
}

void DisplayManager::render(const DisplaySnapshot& snapshot, bool force) {
  (void)snapshot;
  (void)force;
  last_render_ms_ = platform::millis();
}

void DisplayManager::updateEffectiveBrightness() {
#if BIKECOMP_AMBIENT_LIGHT
  effective_brightness_pct_ =
      ambient_valid_
          ? static_cast<uint8_t>((manual_brightness_pct_ * ambient_brightness_pct_) / 100u)
          : manual_brightness_pct_;
#else
  effective_brightness_pct_ = manual_brightness_pct_;
#endif
}

}  // namespace bike
