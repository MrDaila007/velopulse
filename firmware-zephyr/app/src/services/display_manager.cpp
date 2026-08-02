#include "display_manager.h"

#include <zephyr/autoconf.h>

#include "ambient_light_model.h"
#include "display_layout.h"
#include "display_profile.h"
#include "platform.h"
#include "u8g2_display.hpp"

extern "C" {
#include "u8g2.h"
}

namespace bike {
namespace {

constexpr uint8_t kDimContrast = 20;
constexpr uint32_t kDisplayTestDurationMs = 2000u;

class U8g2Canvas final : public DisplayCanvas {
 public:
  explicit U8g2Canvas(U8g2Display& display) : display_(display) {}

  void setFont(DisplayFont font) override {
    switch (font) {
      case DisplayFont::kSpeed:
        display_.setFont(u8g2_font_logisoso20_tn);
        break;
      case DisplayFont::kSpeedLarge:
        display_.setFont(u8g2_font_logisoso38_tn);
        break;
      case DisplayFont::kMetricLarge:
        display_.setFont(u8g2_font_6x13_tf);
        break;
      case DisplayFont::kSmall:
      default:
        display_.setFont(u8g2_font_5x8_tf);
        break;
    }
  }
  void setDrawColor(uint8_t color) override { display_.setDrawColor(color); }
  void drawText(int16_t x, int16_t y, const char* text) override {
    display_.drawStr(x, y, text);
  }
  void drawFrame(int16_t x, int16_t y, uint8_t width, uint8_t height) override {
    display_.drawFrame(x, y, width, height);
  }
  void drawBox(int16_t x, int16_t y, uint8_t width, uint8_t height) override {
    display_.drawBox(x, y, width, height);
  }

 private:
  U8g2Display& display_;
};

}  // namespace

DisplayManager::DisplayManager() = default;

bool DisplayManager::begin(const DeviceConfig& config) {
  display_ok_ = u8g2ZephyrI2cProbe(kDisplayI2cAddress);
  if (!display_ok_) return false;

  display_.setI2CAddress(static_cast<uint8_t>(kDisplayI2cAddress << 1u));
  display_.begin();
  const uint32_t now = millis();
  manual_brightness_pct_ = config.brightness_pct;
  updateEffectiveBrightness();
  carousel_.configure(config, now);
  power_.configure(config.display_auto_off ? config.display_timeout_s : 0u, now);
  burn_in_.configure(now);
  display_.setContrast(bright_contrast_);
  display_.clearBuffer();
  display_.setFont(u8g2_font_6x10_tf);
  if constexpr (kDisplayHeight == 64) {
    display_.drawStr(0, 14, "BikeComp FW " CONFIG_FW_VERSION);
    display_.drawStr(0, 32, "OLED: 128x64");
    display_.drawStr(0, 50, "Reed: D0 drive + D1");
  } else {
    display_.drawStr(0, 12, "BikeComp FW " CONFIG_FW_VERSION);
    display_.drawStr(0, 27, "Reed: D0 + D1");
  }
  display_.sendBuffer();
  return display_ok_;
}

void DisplayManager::applyRuntimeConfig(const DeviceConfig& config,
                                        uint32_t now_ms) {
  manual_brightness_pct_ = config.brightness_pct;
  updateEffectiveBrightness();
  carousel_.configure(config, now_ms);
  power_.configure(config.display_auto_off ? config.display_timeout_s : 0u,
                   now_ms);
  applyPowerHardware();
}

void DisplayManager::noteActivity(uint32_t now_ms) {
  if (!power_.noteActivity(now_ms) || !display_ok_) return;
  applyPowerHardware();
  last_render_ms_ = 0;
}

void DisplayManager::turnOff(uint32_t now_ms) {
  test_active_ = false;
  power_.forceOff(now_ms);
  applyPowerHardware();
}

void DisplayManager::showTestPattern(uint8_t pattern, uint32_t now_ms) {
  if (!display_ok_) return;
  power_.noteActivity(now_ms);
  applyPowerHardware();
  display_.clearBuffer();
  if (pattern == 0) {
    display_.drawBox(0, 0, kDisplayWidth, kDisplayHeight);
  } else if (pattern == 1) {
    for (uint8_t y = 0; y < kDisplayHeight; y += 4) {
      for (uint8_t x = (y / 4u) % 2u == 0 ? 0 : 4; x < kDisplayWidth; x += 8) {
        display_.drawBox(x, y, 4, 4);
      }
    }
  } else {
    display_.setFont(u8g2_font_6x10_tf);
    if constexpr (kDisplayHeight == 64) {
      display_.drawStr(0, 16, "BikeComp display");
      display_.drawStr(0, 36, "SSD1306 128x64");
      display_.drawStr(0, 56, "TEST: OK 012345");
    } else {
      display_.drawStr(0, 12, "BikeComp display");
      display_.drawStr(0, 27, "TEST: OK 012345");
    }
  }
  display_.sendBuffer();
  test_started_ms_ = now_ms;
  test_active_ = true;
}

bool DisplayManager::updatePower(uint32_t now_ms) {
  if (test_active_ &&
      static_cast<uint32_t>(now_ms - test_started_ms_) <
          kDisplayTestDurationMs) {
    return false;
  }
  test_active_ = false;
  if (!power_.update(now_ms)) return false;
  applyPowerHardware();
  return true;
}

void DisplayManager::setAmbientBrightness(uint8_t brightness_pct, bool valid) {
  if (brightness_pct > 100u) brightness_pct = 100u;
  if (brightness_pct < 1u) brightness_pct = 1u;
  if (ambient_brightness_pct_ == brightness_pct && ambient_valid_ == valid) {
    return;
  }
  ambient_brightness_pct_ = brightness_pct;
  ambient_valid_ = valid;
  const uint8_t previous = effective_brightness_pct_;
  updateEffectiveBrightness();
  if (previous != effective_brightness_pct_) applyPowerHardware();
}

void DisplayManager::updateEffectiveBrightness() {
  effective_brightness_pct_ = manual_brightness_pct_;
  if (ambient_valid_ && ambient_brightness_pct_ < effective_brightness_pct_) {
    effective_brightness_pct_ = ambient_brightness_pct_;
  }
  bright_contrast_ =
      AmbientLightModel::contrastForBrightness(effective_brightness_pct_);
}

void DisplayManager::applyPowerHardware() {
  if (!display_ok_) return;
  if (power_.state() == DisplayPowerState::kBright) {
    display_.setPowerSave(0);
    display_.setContrast(bright_contrast_);
  } else if (power_.state() == DisplayPowerState::kDim) {
    display_.setPowerSave(0);
    display_.setContrast(bright_contrast_ < kDimContrast ? bright_contrast_
                                                         : kDimContrast);
  } else {
    display_.setPowerSave(1);
  }
}

void DisplayManager::render(const DisplaySnapshot& snapshot, bool force) {
  const uint32_t now = millis();
  if (test_active_ &&
      static_cast<uint32_t>(now - test_started_ms_) < kDisplayTestDurationMs) {
    return;
  }
  test_active_ = false;
  if (!display_ok_ || power_.state() == DisplayPowerState::kOff) return;
  const bool shift_changed = burn_in_.update(now);
  const bool page_changed = carousel_.update(now);
  const uint32_t period =
      snapshot.trip.ride_state == RideState::kMoving ? 250u : 1000u;
  if (!force && !shift_changed && !page_changed &&
      static_cast<uint32_t>(now - last_render_ms_) < period) {
    return;
  }
  last_render_ms_ = now;

  const bool low_battery_warning =
      snapshot.battery.low_battery && (now % 4000u) >= 3000u;
  const DisplayFrame frame = DisplayFormatter::format(
      snapshot, carousel_.currentPage(), low_battery_warning);
  display_.clearBuffer();
  U8g2Canvas canvas(display_);
  constexpr DisplayProfile kProfile =
      kDisplayHeight == 64 ? DisplayProfile::k128x64
                           : DisplayProfile::k128x32;
  drawDisplayFrame(canvas, frame, kProfile, burn_in_.xOffset(),
                   burn_in_.yOffset());
  display_.sendBuffer();
}

}  // namespace bike
