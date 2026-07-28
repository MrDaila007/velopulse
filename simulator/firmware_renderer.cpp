#include <iostream>
#include <string>

#include "display_formatter.h"
#include "display_layout.h"

namespace {
using namespace bike;

class CommandCanvas final : public DisplayCanvas {
 public:
  void setFont(DisplayFont font) override {
    std::cout << "FONT\t" << (font == DisplayFont::kSpeed ? "SPEED" : "SMALL") << '\n';
  }
  void setDrawColor(uint8_t color) override {
    std::cout << "COLOR\t" << static_cast<unsigned>(color) << '\n';
  }
  void drawText(int16_t x, int16_t y, const char* text) override {
    std::cout << "TEXT\t" << x << '\t' << y << '\t' << text << '\n';
  }
  void drawFrame(int16_t x, int16_t y, uint8_t width, uint8_t height) override {
    shape("FRAME", x, y, width, height);
  }
  void drawBox(int16_t x, int16_t y, uint8_t width, uint8_t height) override {
    shape("BOX", x, y, width, height);
  }

 private:
  static void shape(const char* name, int16_t x, int16_t y, uint8_t width,
                    uint8_t height) {
    std::cout << name << '\t' << x << '\t' << y << '\t'
              << static_cast<unsigned>(width) << '\t'
              << static_cast<unsigned>(height) << '\n';
  }
};

bool makeScenario(const std::string& name, DisplaySnapshot& snapshot, DisplayPage& page) {
  snapshot.trip.speed_x100 = 2489;
  snapshot.trip.trip_distance_mm = 18420000u;
  snapshot.trip.average_speed_x100 = 1975;
  snapshot.trip.max_speed_x100 = 4239;
  snapshot.trip.moving_time_ms = 4356000u;
  snapshot.trip.odometer_mm = 1234500000ull;
  snapshot.trip.ride_state = RideState::kMoving;
  snapshot.battery.valid = true;
  snapshot.battery.percent = 82;

  if (name == "trip" || name == "moving") page = DisplayPage::kTrip;
  else if (name == "average") page = DisplayPage::kAverage;
  else if (name == "maximum") page = DisplayPage::kMaximum;
  else if (name == "time") page = DisplayPage::kMovingTime;
  else if (name == "odometer") page = DisplayPage::kOdometer;
  else if (name == "idle") {
    page = DisplayPage::kTrip;
    snapshot.trip.speed_x100 = 0;
    snapshot.trip.trip_distance_mm = 0;
    snapshot.trip.ride_state = RideState::kIdle;
  } else if (name == "paused") {
    page = DisplayPage::kTrip;
    snapshot.trip.speed_x100 = 0;
    snapshot.trip.ride_state = RideState::kPaused;
  } else if (name == "battery_unknown") {
    page = DisplayPage::kTrip;
    snapshot.battery.valid = false;
  } else if (name == "low_battery") {
    page = DisplayPage::kTrip;
    snapshot.battery.percent = 18;
    snapshot.battery.low_battery = true;
  } else {
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: firmware_renderer SCENARIO\n";
    return 2;
  }
  bike::DisplaySnapshot snapshot;
  bike::DisplayPage page = bike::DisplayPage::kTrip;
  if (!makeScenario(argv[1], snapshot, page)) {
    std::cerr << "unknown scenario: " << argv[1] << '\n';
    return 2;
  }
  const bool low_battery_warning = std::string(argv[1]) == "low_battery";
  const bike::DisplayFrame frame =
      bike::DisplayFormatter::format(snapshot, page, low_battery_warning);
  CommandCanvas canvas;
  bike::drawDisplayFrame(canvas, frame);
  return 0;
}
