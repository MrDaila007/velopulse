#include "board_leds.h"

#include "board_pins.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#ifndef BIKECOMP_SUPPRESS_BOARD_LEDS
#define BIKECOMP_SUPPRESS_BOARD_LEDS 1
#endif

#ifndef BIKECOMP_STATUS_LED_PULSE_MS
#define BIKECOMP_STATUS_LED_PULSE_MS 4
#endif

#ifndef BIKECOMP_STATUS_LED_INTERVAL_MS
#define BIKECOMP_STATUS_LED_INTERVAL_MS 12000
#endif

namespace bike {
namespace {

const struct device* gpio0Dev() { return DEVICE_DT_GET(DT_NODELABEL(gpio0)); }

// XIAO RGB: LOW = on, HIGH = off.
constexpr gpio_flags_t kLedOffLevel = GPIO_OUTPUT_HIGH;
constexpr gpio_flags_t kLedOnLevel = GPIO_OUTPUT_LOW;

void setLedPin(uint8_t pin, gpio_flags_t level) {
  const struct device* dev = gpio0Dev();
  if (!device_is_ready(dev)) return;
  gpio_pin_configure(dev, pin, GPIO_OUTPUT);
  gpio_pin_set(dev, pin, level == GPIO_OUTPUT_LOW ? 0 : 1);
}

void setBlueLedOff() { setLedPin(kBoardLedBluePin, kLedOffLevel); }

}  // namespace

void suppressBoardLeds() {
  setLedPin(kBoardLedRedPin, kLedOffLevel);
  setLedPin(kBoardLedGreenPin, kLedOffLevel);
  setBlueLedOff();
  setLedPin(kBoardChargeIndicatorPin, GPIO_OUTPUT_HIGH);
}

void beginBoardLeds() { suppressBoardLeds(); }

void restoreBoardChargeIndicator() {
#if !BIKECOMP_SUPPRESS_BOARD_LEDS
  const struct device* dev = gpio0Dev();
  if (!device_is_ready(dev)) return;
  gpio_pin_configure(dev, kBoardChargeIndicatorPin, GPIO_INPUT);
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
    setLedPin(kBoardLedBluePin, kLedOnLevel);
  }
}

}  // namespace bike
