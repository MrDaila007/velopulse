#pragma once

#include <stdint.h>

namespace bike {

// Zephyr devicetree aliases (see super_nrf52840.overlay).
// Logical names mirror firmware/include/board_pins.h for review parity.
constexpr uint8_t kHallDtAlias = 0;          // hall-sensor
constexpr uint8_t kAmbientAdcDtAlias = 0;    // ambient-adc
constexpr uint8_t kAmbientPowerDtAlias = 0;  // ambient-power
constexpr uint8_t kBatteryAdcDtAlias = 1;    // battery-adc

// Legacy symbolic names used by ported service code.
constexpr uint8_t kHallPin = kHallDtAlias;
constexpr uint8_t kAmbientLightAdcPin = kAmbientAdcDtAlias;
constexpr uint8_t kAmbientLightPowerPin = kAmbientPowerDtAlias;
constexpr uint8_t kBatteryAdcPin = kBatteryAdcDtAlias;

}  // namespace bike
