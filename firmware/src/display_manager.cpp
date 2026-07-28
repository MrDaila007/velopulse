#include "display_manager.h"

#include "display_layout.h"

namespace bike {
namespace {

class U8g2Canvas final : public DisplayCanvas {
 public:
  explicit U8g2Canvas(U8G2& display) : display_(display) {}

  void setFont(DisplayFont font) override {
    display_.setFont(font == DisplayFont::kSpeed ? u8g2_font_logisoso20_tn
                                                 : u8g2_font_5x8_tf);
  }
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
  carousel_.configure(config, millis());
  if (display_ok_) {
    display_.setContrast(156);
    display_.clearBuffer();
    display_.setFont(u8g2_font_6x10_tf);
    display_.drawStr(0, 12, "BikeComp FW " FW_VERSION);
    display_.drawStr(0, 27, "Button: D0 -> GND");
    display_.sendBuffer();
  }
  return display_ok_;
}

void DisplayManager::render(const DisplaySnapshot& snapshot, bool force) {
  if (!display_ok_) return;
  const uint32_t now = millis();
  const bool page_changed = carousel_.update(now);
  const uint32_t period = snapshot.trip.ride_state == RideState::kMoving ? 250u : 1000u;
  if (!force && !page_changed && static_cast<uint32_t>(now - last_render_ms_) < period) return;
  last_render_ms_ = now;

  const DisplayFrame frame = DisplayFormatter::format(snapshot, carousel_.currentPage());
  display_.clearBuffer();
  U8g2Canvas canvas(display_);
  drawDisplayFrame(canvas, frame);
  display_.sendBuffer();
}

}  // namespace bike
