#include "ambient_light_manager.h"

#include "board_leds.h"
#include "board_pins.h"

namespace bike {
namespace {

constexpr uint32_t kSamplePeriodMs = 1000u;
constexpr uint32_t kSettleTimeMs = 10u;
constexpr uint8_t kSampleCount = 16u;
constexpr uint16_t kPresenceMinimumRaw = 4u;

}  // namespace

void AmbientLightManager::begin(uint32_t now_ms) {
  model_.configure(BIKECOMP_AMBIENT_RAW_DARK,
                   BIKECOMP_AMBIENT_RAW_BRIGHT, now_ms);
#if BIKECOMP_AMBIENT_LIGHT
  pinMode(kAmbientLightPowerPin, OUTPUT);
  digitalWrite(kAmbientLightPowerPin, LOW);
  pinMode(kAmbientLightAdcPin, INPUT);
  analogReference(AR_INTERNAL_2_4);
  analogReadResolution(12);
  analogSampleTime(40);
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
    // A powered divider overcomes this weak pull-down; an absent circuit does not.
    pinMode(kAmbientLightAdcPin, INPUT_PULLDOWN);
    presence_checked_ = false;
    suppressBoardLeds();
    digitalWrite(kAmbientLightPowerPin, HIGH);
    power_started_ms_ = now_ms;
    powered_ = true;
    return false;
  }

  if (static_cast<uint32_t>(now_ms - power_started_ms_) < kSettleTimeMs) {
    return false;
  }

  if (!presence_checked_) {
    const uint16_t presence_raw =
        static_cast<uint16_t>(analogRead(kAmbientLightAdcPin));
    if (presence_raw <= kPresenceMinimumRaw) {
      digitalWrite(kAmbientLightPowerPin, LOW);
      restoreBoardChargeIndicator();
      pinMode(kAmbientLightAdcPin, INPUT);
      powered_ = false;
      last_sample_ms_ = now_ms;
      return model_.addSample(0u, now_ms);
    }
    pinMode(kAmbientLightAdcPin, INPUT);
    power_started_ms_ = now_ms;
    presence_checked_ = true;
    return false;
  }

  uint16_t minimum = kAmbientAdcMaximum;
  uint16_t maximum = 0;
  uint32_t sum = 0;
  for (uint8_t i = 0; i < kSampleCount; ++i) {
    const uint16_t raw = static_cast<uint16_t>(analogRead(kAmbientLightAdcPin));
    if (raw < minimum) minimum = raw;
    if (raw > maximum) maximum = raw;
    sum += raw;
  }
  digitalWrite(kAmbientLightPowerPin, LOW);
  restoreBoardChargeIndicator();
  powered_ = false;
  presence_checked_ = false;
  last_sample_ms_ = now_ms;
  const uint16_t average =
      static_cast<uint16_t>((sum - minimum - maximum + 7u) / 14u);
  return model_.addSample(average, now_ms);
#endif
}

}  // namespace bike
