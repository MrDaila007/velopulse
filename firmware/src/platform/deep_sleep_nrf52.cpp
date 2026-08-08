#include "platform/deep_sleep.h"

#include <Arduino.h>
#include <nrf_gpio.h>
#include <nrf_power.h>
#include <nrf_soc.h>

#include "board_pins.h"

namespace bike {
namespace {

char g_wake_source[8] = "reset";

void setWakeSource(const char* value) {
  for (size_t i = 0; i < sizeof(g_wake_source); ++i) {
    g_wake_source[i] = value[i];
    if (value[i] == '\0') break;
  }
}

}  // namespace

bool deepSleepWakeFromSleep() {
  const uint32_t reason = NRF_POWER->RESETREAS;
  if ((reason & POWER_RESETREAS_OFF_Msk) != 0u) {
    setWakeSource("hall");
    return true;
  }
  if ((reason & POWER_RESETREAS_VBUS_Msk) != 0u) {
    setWakeSource("usb");
    return true;
  }
  return false;
}

bool deepSleepPrepareAndEnter(uint32_t hall_nrf_gpio, bool sense_low) {
#if BIKECOMP_HALL_TWO_WIRE
  pinMode(kHallDrivePin, INPUT);
  nrf_gpio_cfg_input(kHallDriveNrfGpio, NRF_GPIO_PIN_NOPULL);
#endif

  pinMode(kHallSensePin, INPUT_PULLUP);
  const auto sense = sense_low ? NRF_GPIO_PIN_SENSE_LOW
                               : NRF_GPIO_PIN_SENSE_HIGH;
  nrf_gpio_cfg_sense_input(hall_nrf_gpio, NRF_GPIO_PIN_PULLUP, sense);

  // USB VBUS wake is handled by the SoC; CHG line is an additional reserve path.
  nrf_gpio_cfg_sense_input(kBoardChargeNrfGpio, NRF_GPIO_PIN_NOPULL,
                           NRF_GPIO_PIN_SENSE_LOW);

  sd_power_system_off();
  return false;
}

const char* deepSleepWakeSourceName() { return g_wake_source; }

}  // namespace bike
