#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ble_protocol.h"
#include "config.h"
#include "config_codec.h"
#include "config_validator.h"

namespace bike {

enum class ConfigWriteQueueState : uint8_t {
  kIdle = 0,
  kPending,
};

enum class ConfigWriteRejectReason : uint8_t {
  kNone = 0,
  kBusy,
  kBadLength,
};

struct ConfigWriteParseResult {
  bool ok = false;
  CommandStatus status = CommandStatus::kOk;
  uint8_t field_id = 0;
  DeviceConfig config = {};
};

struct PendingConfigWrite {
  uint8_t data[kConfigurationSize] = {};
  ConfigWriteQueueState state = ConfigWriteQueueState::kIdle;
};

uint8_t configValidationErrorToFieldId(ConfigValidationError error);
uint8_t configWireErrorToFieldId(DeviceConfigWireError error);

ConfigWriteParseResult parseConfigWritePayload(const uint8_t* data, size_t length);

bool configWriteQueueStage(PendingConfigWrite& queue,
                           const uint8_t* data,
                           size_t length,
                           ConfigWriteRejectReason& reject);
bool configWriteQueueDequeue(PendingConfigWrite& queue,
                             uint8_t out[kConfigurationSize]);
void configWriteQueueFinish(PendingConfigWrite& queue);

}  // namespace bike
