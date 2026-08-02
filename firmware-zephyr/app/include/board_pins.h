#pragma once

#include <stdint.h>

namespace bike {

// Zephyr devicetree aliases (see super_nrf52840.overlay).
constexpr uint8_t kHallDtAlias = 0;          // hall-sensor
constexpr uint8_t kHallDriveDtAlias = 0;     // hall-drive
constexpr uint8_t kAmbientAdcDtAlias = 0;    // ambient-adc-mux
constexpr uint8_t kAmbientPowerDtAlias = 0;  // ambient-power
constexpr uint8_t kBatteryAdcDtAlias = 1;    // battery-adc-mux

// Logical names mirroring firmware/include/board_pins.h.
constexpr uint8_t kHallDrivePin = kHallDriveDtAlias;
constexpr uint8_t kHallSensePin = kHallDtAlias;
constexpr uint8_t kHallPin = kHallSensePin;
constexpr uint8_t kAmbientLightAdcPin = kAmbientAdcDtAlias;
constexpr uint8_t kAmbientLightPowerPin = kAmbientPowerDtAlias;
constexpr uint8_t kBatteryAdcPin = kBatteryAdcDtAlias;

constexpr char kHallPinLabel[] = "D0+D1";
constexpr char kHallSensePinLabel[] = "D1/P0.03";
constexpr char kHallDrivePinLabel[] = "D0/P0.02";
constexpr char kHallNrfPin[] = "sense=P0.03 drive=P0.02";

// XIAO nRF52840 board LEDs (active-low RGB) and charge indicator.
constexpr uint8_t kBoardLedRedPin = 26;    // P0.26 / D11
constexpr uint8_t kBoardLedBluePin = 6;    // P0.06 / D12
constexpr uint8_t kBoardLedGreenPin = 30;  // P0.30 / D13
constexpr uint8_t kBoardChargeIndicatorPin = 17;  // P0.17 / D23

}  // namespace bike
