#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config_codec.h"
#include "config.h"

namespace bike {

enum class LoadConfigResult : uint8_t {
  kOk = 0,
  kWireDecode,
  kValidation,
  kStorage,
};

const char* loadConfigResultMessage(LoadConfigResult result);
LoadConfigResult decodeConfigWire(const uint8_t payload[kDeviceConfigPayloadSize],
                                  DeviceConfig& parsed);

}  // namespace bike
