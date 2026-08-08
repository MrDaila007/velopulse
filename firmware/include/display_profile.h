#pragma once

#include <stdint.h>

#ifndef BIKECOMP_DISPLAY_HEIGHT
#define BIKECOMP_DISPLAY_HEIGHT 64
#endif

#if BIKECOMP_DISPLAY_HEIGHT != 32 && BIKECOMP_DISPLAY_HEIGHT != 64
#error "BIKECOMP_DISPLAY_HEIGHT must be 32 or 64"
#endif

namespace bike {

constexpr uint8_t kDisplayWidth = 128;
constexpr uint8_t kDisplayHeight = BIKECOMP_DISPLAY_HEIGHT;

}  // namespace bike
