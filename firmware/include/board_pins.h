#pragma once

#include <Arduino.h>

namespace bike {

constexpr uint8_t kHallDrivePin = D0;   // Arduino D0 = nRF P0.02
constexpr uint8_t kHallSensePin = D1;   // Arduino D1 = nRF P0.03 (pad A1/D1)
constexpr uint32_t kHallSenseNrfGpio = 3u;   // P0.03
constexpr uint32_t kHallDriveNrfGpio = 2u;   // P0.02
constexpr uint8_t kHallPin = kHallSensePin;
constexpr uint8_t kHallAdcPin = A1;
constexpr char kHallPinLabel[] = "D0+D1";
constexpr char kHallSensePinLabel[] = "D1/P0.03";
constexpr char kHallDrivePinLabel[] = "D0/P0.02";
constexpr char kHallNrfPin[] = "sense=P0.03 drive=P0.02";
constexpr uint8_t kAmbientLightAdcPin = A2;
constexpr uint8_t kAmbientLightPowerPin = D3;
constexpr uint8_t kDisplaySdaPin = D4;
constexpr uint8_t kDisplaySclPin = D5;
// XIAO nRF52840: active-low RGB on D11/D12/D13; ~CHG on D23 (P0.17) drives charge LED.
constexpr uint8_t kBoardChargeIndicatorPin = 23;
constexpr uint32_t kBoardChargeNrfGpio = 17u;  // P0.17
// External divider for Super-nRF52840:
// BAT -> 1 MOhm -> P0.31 (rear pad / logical pin 32) -> 1 MOhm -> GND.
constexpr uint8_t kBatteryAdcPin = 32;

}  // namespace bike
