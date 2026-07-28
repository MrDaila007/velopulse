#include "display_manager.h"

#include <stdio.h>

#include "config.h"

namespace bike {

DisplayManager::DisplayManager() : display_(U8G2_R0, U8X8_PIN_NONE) {}

bool DisplayManager::begin() {
  Wire.begin();
  Wire.setClock(400000);
  display_.setI2CAddress(kDisplayI2cAddress << 1u);
  display_ok_ = display_.begin();
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

void DisplayManager::render(const TripSnapshot& snapshot, bool force) {
  if (!display_ok_) return;
  const uint32_t now = millis();
  const uint32_t period = snapshot.ride_state == RideState::kMoving ? 250u : 1000u;
  if (!force && static_cast<uint32_t>(now - last_render_ms_) < period) return;
  last_render_ms_ = now;

  char speed[16];
  char lower[24];
  snprintf(speed, sizeof(speed), "%u.%u", snapshot.speed_x100 / 100u,
           (snapshot.speed_x100 % 100u) / 10u);
  const char* state = snapshot.ride_state == RideState::kMoving
                          ? "MOV"
                          : (snapshot.ride_state == RideState::kPaused ? "PAUSE" : "IDLE");
  snprintf(lower, sizeof(lower), "%s  TRIP %lu.%02lu", state,
           static_cast<unsigned long>(snapshot.trip_distance_mm / 1000000u),
           static_cast<unsigned long>((snapshot.trip_distance_mm / 10000u) % 100u));

  display_.clearBuffer();
  display_.setFont(u8g2_font_logisoso20_tn);
  display_.drawStr(0, 21, speed);
  display_.setFont(u8g2_font_5x8_tf);
  display_.drawStr(88, 20, "km/h");
  display_.drawStr(0, 31, lower);
  display_.sendBuffer();
}

}  // namespace bike
