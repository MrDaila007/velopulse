#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bike {

uint32_t crc32(const uint8_t* data, size_t length);

}  // namespace bike
