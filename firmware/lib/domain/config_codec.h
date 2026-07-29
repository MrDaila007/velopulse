#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

namespace bike {

constexpr uint8_t kDeviceConfigVersion = 1;
constexpr size_t kDeviceConfigPayloadSize = 48;

void encodeDeviceConfig(const DeviceConfig& config,
                        uint8_t output[kDeviceConfigPayloadSize]);
bool decodeDeviceConfig(const uint8_t* input,
                        size_t length,
                        DeviceConfig& config);
bool deviceConfigsEqual(const DeviceConfig& left, const DeviceConfig& right);

}  // namespace bike
