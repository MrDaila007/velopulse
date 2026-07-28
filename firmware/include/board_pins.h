#pragma once

#include <Arduino.h>

namespace bike {

constexpr uint8_t kHallPin = D0;
constexpr uint8_t kDisplaySdaPin = D4;
constexpr uint8_t kDisplaySclPin = D5;
// Super-nRF52840: BAT -> 1 MOhm -> P0.03/D1 -> 510 kOhm -> P0.04/D4.
// D4 is shared with OLED SDA and is driven LOW only while the I2C bus is paused.
constexpr uint8_t kBatteryAdcPin = D1;
constexpr uint8_t kBatteryDividerLowPin = D4;

}  // namespace bike
