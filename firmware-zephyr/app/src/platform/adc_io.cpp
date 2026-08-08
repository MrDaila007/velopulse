#include "adc_io.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "zephyr,user io-channels missing in devicetree overlay"
#endif

namespace bike {
namespace adc_io {
namespace {

static const struct adc_dt_spec kAmbientAdc =
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const struct adc_dt_spec kBatteryAdc =
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);

bool setupOnce = false;

bool ensureSetup() {
  if (setupOnce) return true;
  if (!adc_is_ready_dt(&kAmbientAdc) || !adc_is_ready_dt(&kBatteryAdc)) {
    return false;
  }
  if (adc_channel_setup_dt(&kAmbientAdc) != 0) return false;
  if (adc_channel_setup_dt(&kBatteryAdc) != 0) return false;
  setupOnce = true;
  return true;
}

bool readSpec(const struct adc_dt_spec& spec, uint16_t& raw_out) {
  if (!ensureSetup()) return false;

  int16_t sample = 0;
  struct adc_sequence sequence = {};
  sequence.buffer = &sample;
  sequence.buffer_size = sizeof(sample);
  if (adc_sequence_init_dt(&spec, &sequence) != 0) return false;
  if (adc_read_dt(&spec, &sequence) != 0) return false;
  if (sample < 0) sample = 0;
  raw_out = static_cast<uint16_t>(sample);
  return true;
}

}  // namespace

bool readChannel(AdcChannel channel, uint16_t& raw_out) {
  switch (channel) {
    case AdcChannel::kAmbient:
      return readSpec(kAmbientAdc, raw_out);
    case AdcChannel::kBattery:
      return readSpec(kBatteryAdc, raw_out);
  }
  return false;
}

}  // namespace adc_io
}  // namespace bike
