#include "ambient_light_manager.h"

#include "adc_io.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#define AMBIENT_POWER_NODE DT_ALIAS(ambient_power)
#define AMBIENT_POWER_PIN_NODE DT_CHILD(AMBIENT_POWER_NODE, ldr_power)

namespace bike {
namespace {

constexpr uint32_t kSamplePeriodMs = 1000u;
constexpr uint32_t kSettleTimeMs = 10u;
constexpr uint8_t kSampleCount = 16u;
constexpr uint16_t kPresenceMinimumRaw = 4u;

static const struct gpio_dt_spec kAmbientPowerGpio =
    GPIO_DT_SPEC_GET(AMBIENT_POWER_PIN_NODE, gpios);

}  // namespace

void AmbientLightManager::begin(uint32_t now_ms) {
  model_.configure(BIKECOMP_AMBIENT_RAW_DARK, BIKECOMP_AMBIENT_RAW_BRIGHT,
                   now_ms);
#if BIKECOMP_AMBIENT_LIGHT
  gpio_pin_configure_dt(&kAmbientPowerGpio, GPIO_OUTPUT_INACTIVE);
  last_sample_ms_ = now_ms - kSamplePeriodMs;
#else
  last_sample_ms_ = now_ms;
#endif
}

bool AmbientLightManager::update(uint32_t now_ms) {
#if !BIKECOMP_AMBIENT_LIGHT
  (void)now_ms;
  return false;
#else
  if (!powered_) {
    if (static_cast<uint32_t>(now_ms - last_sample_ms_) < kSamplePeriodMs) {
      return false;
    }
    presence_checked_ = false;
    gpio_pin_set_dt(&kAmbientPowerGpio, 1);
    power_started_ms_ = now_ms;
    powered_ = true;
    return false;
  }

  if (static_cast<uint32_t>(now_ms - power_started_ms_) < kSettleTimeMs) {
    return false;
  }

  if (!presence_checked_) {
    uint16_t presence_raw = 0;
    if (!adc_io::readChannel(adc_io::AdcChannel::kAmbient, presence_raw) ||
        presence_raw <= kPresenceMinimumRaw) {
      gpio_pin_set_dt(&kAmbientPowerGpio, 0);
      powered_ = false;
      last_sample_ms_ = now_ms;
      return model_.addSample(0u, now_ms);
    }
    power_started_ms_ = now_ms;
    presence_checked_ = true;
    return false;
  }

  uint16_t minimum = kAmbientAdcMaximum;
  uint16_t maximum = 0;
  uint32_t sum = 0;
  for (uint8_t i = 0; i < kSampleCount; ++i) {
    uint16_t raw = 0;
    if (!adc_io::readChannel(adc_io::AdcChannel::kAmbient, raw)) {
      gpio_pin_set_dt(&kAmbientPowerGpio, 0);
      powered_ = false;
      last_sample_ms_ = now_ms;
      return false;
    }
    if (raw < minimum) minimum = raw;
    if (raw > maximum) maximum = raw;
    sum += raw;
  }
  gpio_pin_set_dt(&kAmbientPowerGpio, 0);
  powered_ = false;
  presence_checked_ = false;
  last_sample_ms_ = now_ms;
  const uint16_t average =
      static_cast<uint16_t>((sum - minimum - maximum + 7u) / 14u);
  return model_.addSample(average, now_ms);
#endif
}

}  // namespace bike
