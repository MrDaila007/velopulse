#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ble_protocol.h"

namespace bike {

// Command packet field offsets (protocol §5); used as Command Result detail on ERR_RANGE.
enum class CommandFieldId : uint8_t {
  kStructVersion = 0,
  kCommandId = 1,
  kFlags = 2,
  kPayloadLen = 3,
  kPayload = 4,
};

constexpr uint8_t kDisplayTestPatternMax = 2;
constexpr uint16_t kSensorTestDurationMinS = 1;
constexpr uint16_t kSensorTestDurationMaxS = 120;

struct SafeCommandParams {
  uint8_t display_test_pattern = 0;
  uint16_t sensor_test_duration_s = 0;
};

struct SafeCommandParseResult {
  bool ok = false;
  CommandStatus status = CommandStatus::kOk;
  uint8_t field_id = 0;
  CommandId command_id = CommandId::kResetTrip;
  SafeCommandParams params = {};
};

bool isSafeCommandId(uint8_t command_id);
SafeCommandParseResult parseSafeBleCommand(const uint8_t* data, size_t length);

}  // namespace bike
