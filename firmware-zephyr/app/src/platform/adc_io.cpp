#include "adc_io.h"

namespace bike {
namespace adc_io {

bool readChannel(AdcChannel channel, uint16_t& raw_out) {
  (void)channel;
  // Z1.3: runtime SAADC setup lands after devicetree channel nodes are finalized.
  raw_out = 0;
  return false;
}

}  // namespace adc_io
}  // namespace bike
