#include "load_config.h"

#include "config_validator.h"

namespace bike {

const char* loadConfigResultMessage(LoadConfigResult result) {
  switch (result) {
    case LoadConfigResult::kOk:
      return "OK load-config";
    case LoadConfigResult::kWireDecode:
      return "ERROR load-config wire";
    case LoadConfigResult::kValidation:
      return "ERROR load-config validation";
    case LoadConfigResult::kStorage:
      return "ERROR load-config storage";
  }
  return "ERROR load-config storage";
}

LoadConfigResult decodeConfigWire(
    const uint8_t payload[kDeviceConfigPayloadSize], DeviceConfig& parsed) {
  const DeviceConfigWireError wire =
      tryDecodeDeviceConfigWire(payload, kDeviceConfigPayloadSize, parsed);
  if (wire != DeviceConfigWireError::kOk) {
    return LoadConfigResult::kWireDecode;
  }
  if (ConfigValidator::validate(parsed) != ConfigValidationError::kNone) {
    return LoadConfigResult::kValidation;
  }
  return LoadConfigResult::kOk;
}

}  // namespace bike
