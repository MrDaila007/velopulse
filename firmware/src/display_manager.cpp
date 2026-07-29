#include "display_manager.h"

#include "display_layout.h"

namespace bike {
namespace {

constexpr uint8_t kBrightContrast = 156;
constexpr uint8_t kDimContrast = 20;

class U8g2Canvas final : public DisplayCanvas {
 public:
  explicit U8g2Canvas(U8G2& display) : display_(display) {}

  void setFont(DisplayFont font) override {
    display_.setFont(font == DisplayFont::kSpeed ? u8g2_font_logisoso20_tn
                                                 : u8g2_font_5x8_tf);
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
  U8G2& display_;
};

}  // namespace

DisplayManager::DisplayManager() : display_(U8G2_R0, U8X8_PIN_NONE) {}

bool DisplayManager::begin(const DeviceConfig& config) {
  Wire.begin();
  Wire.setClock(400000);
  display_.setI2CAddress(kDisplayI2cAddress << 1u);
  display_ok_ = display_.begin();
  const uint32_t now = millis();
  carousel_.configure(config, now);
  power_.configure(config.display_timeout_s, now);
  if (display_ok_) {
    display_.setContrast(kBrightContrast);
    display_.clearBuffer();
    display_.setFont(u8g2_font_6x10_tf);
    display_.drawStr(0, 12, "BikeComp FW " FW_VERSION);
    display_.drawStr(0, 27, "Button: D0 -> GND");
    display_.sendBuffer();
  }
  return display_ok_;
}

void DisplayManager::noteActivity(uint32_t now_ms) {
  if (!power_.noteActivity(now_ms) || !display_ok_) return;
  applyPowerHardware();
  last_render_ms_ = 0;
}

bool DisplayManager::updatePower(uint32_t now_ms) {
  if (!power_.update(now_ms)) return false;
  applyPowerHardware();
  return true;
}

void DisplayManager::applyPowerHardware() {
  if (!display_ok_) return;
  if (power_.state() == DisplayPowerState::kBright) {
    display_.setPowerSave(0);
    display_.setContrast(kBrightContrast);
  } else if (power_.state() == DisplayPowerState::kDim) {
    display_.setPowerSave(0);
    display_.setContrast(kDimContrast);
  } else {
    display_.setPowerSave(1);
  }
}

void DisplayManager::render(const DisplaySnapshot& snapshot, bool force) {
  const uint32_t now = millis();
  if (!display_ok_ || power_.state() == DisplayPowerState::kOff) return;
  const bool page_changed = carousel_.update(now);
  const uint32_t period = snapshot.trip.ride_state == RideState::kMoving ? 250u : 1000u;
  if (!force && !page_changed && static_cast<uint32_t>(now - last_render_ms_) < period) return;
  last_render_ms_ = now;

  const bool low_battery_warning =
      snapshot.battery.low_battery && (now % 4000u) >= 3000u;
  const DisplayFrame frame = DisplayFormatter::format(
      snapshot, carousel_.currentPage(), low_battery_warning);
  display_.clearBuffer();
  U8g2Canvas canvas(display_);
  drawDisplayFrame(canvas, frame);
  display_.sendBuffer();
}

}  // namespace bike
