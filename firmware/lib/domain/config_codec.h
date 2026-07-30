#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

namespace bike {

constexpr uint8_t kDeviceConfigVersion = 1;
constexpr size_t kDeviceConfigPayloadSize = 48;

enum class DeviceConfigWireError : uint8_t {
  kOk = 0,
  kBadLength,
  kBadVersion,
  kBadPadding,
};

void encodeDeviceConfig(const DeviceConfig& config,
                        uint8_t output[kDeviceConfigPayloadSize]);
DeviceConfigWireError tryDecodeDeviceConfigWire(const uint8_t* input,
                                                size_t length,
                                                DeviceConfig& config);
bool decodeDeviceConfig(const uint8_t* input,
                        size_t length,
                        DeviceConfig& config);
bool deviceConfigsEqual(const DeviceConfig& left, const DeviceConfig& right);

}  // namespace bike
