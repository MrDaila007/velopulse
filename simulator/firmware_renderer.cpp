#include <cstdio>
#include <iostream>
#include <string>

#include "display_formatter.h"
#include "display_layout.h"

namespace {
using namespace bike;

class CommandCanvas final : public DisplayCanvas {
 public:
  void setFont(DisplayFont font) override {
    const char* name = "SMALL";
    switch (font) {
      case DisplayFont::kSpeed:
        name = "SPEED";
        break;
      case DisplayFont::kSpeedLarge:
        name = "SPEED_LARGE";
        break;
      case DisplayFont::kMetricLarge:
        name = "METRIC_LARGE";
        break;
      case DisplayFont::kSmall:
      default:
        break;
    }
    std::cout << "FONT\t" << name << '\n';
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

bool makeScenario(const std::string& name, DisplaySnapshot& snapshot,
                  DisplayPage& page) {
  snapshot.trip.speed_x100 = 2489;
  snapshot.trip.trip_distance_mm = 18420000u;
  snapshot.trip.average_speed_x100 = 1975;
  snapshot.trip.max_speed_x100 = 4239;
  snapshot.trip.moving_time_ms = 4356000u;
  snapshot.trip.odometer_mm = 1234500000ull;
  snapshot.trip.ride_state = RideState::kMoving;
  snapshot.battery.valid = true;
  snapshot.battery.percent = 82;
  snapshot.battery.millivolts = 3900;

  if (name == "trip" || name == "moving") page = DisplayPage::kTrip;
  else if (name == "average") page = DisplayPage::kAverage;
  else if (name == "maximum") page = DisplayPage::kMaximum;
  else if (name == "time") page = DisplayPage::kMovingTime;
  else if (name == "odometer") page = DisplayPage::kOdometer;
  else if (name == "weather_clock") {
    page = DisplayPage::kWeatherClock;
    snapshot.companion_header_valid = true;
    std::snprintf(snapshot.companion_header, sizeof(snapshot.companion_header),
                  "14:30");
  } else if (name == "weather_rain") {
    page = DisplayPage::kWeatherRain;
    snapshot.companion_weather_valid = true;
    std::snprintf(snapshot.companion_weather_temp,
                  sizeof(snapshot.companion_weather_temp), "+18.5C");
    std::snprintf(snapshot.companion_weather_rain,
                  sizeof(snapshot.companion_weather_rain), "R40%%");
  } else if (name == "idle") {
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
  } else if (name == "ble_connected") {
    page = DisplayPage::kTrip;
    snapshot.ble_connected = true;
  } else if (name == "speed_0") {
    page = DisplayPage::kTrip;
    snapshot.trip.speed_x100 = 0;
  } else if (name == "speed_99_9") {
    page = DisplayPage::kTrip;
    snapshot.trip.speed_x100 = 9990;
  } else if (name == "speed_100") {
    page = DisplayPage::kTrip;
    snapshot.trip.speed_x100 = 10000;
  } else if (name == "speed_200") {
    page = DisplayPage::kTrip;
    snapshot.trip.speed_x100 = 20000;
  } else if (name == "battery_empty") {
    page = DisplayPage::kTrip;
    snapshot.battery.percent = 0;
    snapshot.battery.millivolts = 3000;
  } else if (name == "battery_full") {
    page = DisplayPage::kTrip;
    snapshot.battery.percent = 100;
    snapshot.battery.millivolts = 4200;
  } else if (name == "long_metric") {
    page = DisplayPage::kTrip;
  } else {
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 5 || argc == 4) {
    std::cerr <<
        "usage: firmware_renderer SCENARIO [32|64 [X_OFFSET Y_OFFSET]]\n";
    return 2;
  }
  bike::DisplaySnapshot snapshot;
  bike::DisplayPage page = bike::DisplayPage::kTrip;
  if (!makeScenario(argv[1], snapshot, page)) {
    std::cerr << "unknown scenario: " << argv[1] << '\n';
    return 2;
  }

  bike::DisplayProfile profile = bike::DisplayProfile::k128x64;
  if (argc >= 3) {
    const std::string height = argv[2];
    if (height == "32") {
      profile = bike::DisplayProfile::k128x32;
    } else if (height != "64") {
      std::cerr << "unsupported display height: " << height << '\n';
      return 2;
    }
  }

  int8_t x_offset = 0;
  int8_t y_offset = 0;
  if (argc == 5) {
    x_offset = static_cast<int8_t>(std::stoi(argv[3]));
    y_offset = static_cast<int8_t>(std::stoi(argv[4]));
    if (x_offset < 0 || x_offset > 1 || y_offset < 0 || y_offset > 1) {
      std::cerr << "display offsets must be 0 or 1\n";
      return 2;
    }
  }

  const bool low_battery_warning = std::string(argv[1]) == "low_battery";
  bike::DisplayFrame frame =
      bike::DisplayFormatter::format(snapshot, page, low_battery_warning);
  if (std::string(argv[1]) == "long_metric") {
    std::snprintf(frame.lower, sizeof(frame.lower),
                  "MOV VERY LONG METRIC VALUE");
  }
  CommandCanvas canvas;
  bike::drawDisplayFrame(canvas, frame, profile, x_offset, y_offset);
  return 0;
}
