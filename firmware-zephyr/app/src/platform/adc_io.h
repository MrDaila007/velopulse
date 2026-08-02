#pragma once

#include <stdint.h>

namespace bike {
namespace adc_io {

// Super-nRF52840 ADC channels (see super_nrf52840.overlay).
enum class AdcChannel : uint8_t {
  kAmbient = 0,  // P1.04 / AIN2
  kBattery = 1,  // P0.31 / AIN5
};

bool readChannel(AdcChannel channel, uint16_t& raw_out);

}  // namespace adc_io
}  // namespace bike
