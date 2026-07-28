#pragma once

#include <Arduino.h>

namespace bike {

constexpr uint8_t kHallPin = D0;
constexpr uint8_t kDisplaySdaPin = D4;
constexpr uint8_t kDisplaySclPin = D5;
// External divider for Super-nRF52840:
// BAT -> 1 MOhm -> P0.31 (rear pad / logical pin 32) -> 510 kOhm -> GND.
constexpr uint8_t kBatteryAdcPin = 32;

}  // namespace bike
