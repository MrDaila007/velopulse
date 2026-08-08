#include "board_leds.h"

#include <Arduino.h>

#include "board_pins.h"

#ifndef BIKECOMP_SUPPRESS_BOARD_LEDS
#define BIKECOMP_SUPPRESS_BOARD_LEDS 0
#endif

#ifndef BIKECOMP_STATUS_LED_PULSE_MS
#define BIKECOMP_STATUS_LED_PULSE_MS 4
#endif

#ifndef BIKECOMP_STATUS_LED_INTERVAL_MS
#define BIKECOMP_STATUS_LED_INTERVAL_MS 12000
#endif

namespace bike {
namespace {

// XIAO RGB: LOW = on, HIGH = off.
constexpr uint8_t kLedOffLevel = HIGH;
constexpr uint8_t kLedOnLevel = LOW;

void setBlueLedOff() {
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, kLedOffLevel);
}

}  // namespace

void suppressBoardLeds() {
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, kLedOffLevel);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, kLedOffLevel);
  setBlueLedOff();
  pinMode(kBoardChargeIndicatorPin, OUTPUT);
  digitalWrite(kBoardChargeIndicatorPin, HIGH);
}

void beginBoardLeds() { suppressBoardLeds(); }

void restoreBoardChargeIndicator() {
#if !BIKECOMP_SUPPRESS_BOARD_LEDS
  pinMode(kBoardChargeIndicatorPin, INPUT);
#endif
}

void updateBoardStatusLed(bool ble_advertising, bool ble_connected,
                          uint32_t now_ms) {
  setBlueLedOff();
#if BIKECOMP_STATUS_LED_INTERVAL_MS == 0 || BIKECOMP_STATUS_LED_PULSE_MS == 0
  (void)ble_advertising;
  (void)ble_connected;
  (void)now_ms;
  return;
#endif
  if (!ble_advertising || ble_connected) return;

  const uint32_t phase =
      now_ms % static_cast<uint32_t>(BIKECOMP_STATUS_LED_INTERVAL_MS);
  if (phase < static_cast<uint32_t>(BIKECOMP_STATUS_LED_PULSE_MS)) {
    digitalWrite(LED_BLUE, kLedOnLevel);
  }
}

}  // namespace bike
