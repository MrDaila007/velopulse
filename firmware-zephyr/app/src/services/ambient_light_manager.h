#pragma once

#include <stdint.h>

#include "ambient_light_model.h"

#ifndef BIKECOMP_AMBIENT_LIGHT
#define BIKECOMP_AMBIENT_LIGHT 1
#endif

#ifndef BIKECOMP_AMBIENT_RAW_DARK
#define BIKECOMP_AMBIENT_RAW_DARK 100
#endif

#ifndef BIKECOMP_AMBIENT_RAW_BRIGHT
#define BIKECOMP_AMBIENT_RAW_BRIGHT 3900
#endif

namespace bike {

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
  bool presence_checked_ = false;
};

}  // namespace bike
