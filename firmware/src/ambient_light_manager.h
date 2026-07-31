#pragma once

#include <Arduino.h>

#include "ambient_light_model.h"

#ifndef BIKECOMP_AMBIENT_LIGHT
#define BIKECOMP_AMBIENT_LIGHT 1
#endif

#if BIKECOMP_AMBIENT_LIGHT != 0 && BIKECOMP_AMBIENT_LIGHT != 1
#error "BIKECOMP_AMBIENT_LIGHT must be 0 or 1"
#endif

#ifndef BIKECOMP_AMBIENT_RAW_DARK
#define BIKECOMP_AMBIENT_RAW_DARK 100
#endif

#ifndef BIKECOMP_AMBIENT_RAW_BRIGHT
#define BIKECOMP_AMBIENT_RAW_BRIGHT 3900
#endif

namespace bike {
#if BIKECOMP_AMBIENT_RAW_DARK < 0 || BIKECOMP_AMBIENT_RAW_DARK > 4095
#error "BIKECOMP_AMBIENT_RAW_DARK must be in ADC range 0..4095"
#endif

#if BIKECOMP_AMBIENT_RAW_BRIGHT > 4095 || \
    BIKECOMP_AMBIENT_RAW_DARK >= BIKECOMP_AMBIENT_RAW_BRIGHT
#error "ambient raw calibration must satisfy dark < bright <= 4095"
#endif

class AmbientLightManager {
 public:
  void begin(uint32_t now_ms);
  bool update(uint32_t now_ms);

  const AmbientLightSnapshot& snapshot() const { return model_.snapshot(); }
  bool enabled() const { return BIKECOMP_AMBIENT_LIGHT != 0; }

 private:
  AmbientLightModel model_;
  uint32_t last_sample_ms_ = 0;
  uint32_t power_started_ms_ = 0;
  bool powered_ = false;
};

}  // namespace bike
