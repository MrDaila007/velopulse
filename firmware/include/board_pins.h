#pragma once

#include <Arduino.h>

namespace bike {

constexpr uint8_t kHallPin = D0;
constexpr uint8_t kAmbientLightAdcPin = A2;
constexpr uint8_t kAmbientLightPowerPin = D3;
constexpr uint8_t kDisplaySdaPin = D4;
constexpr uint8_t kDisplaySclPin = D5;
// External divider for Super-nRF52840:
// BAT -> 1 MOhm -> P0.31 (rear pad / logical pin 32) -> 1 MOhm -> GND.
constexpr uint8_t kBatteryAdcPin = 32;

}  // namespace bike
