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

enum class SafeCommandQueueState : uint8_t {
  kIdle = 0,
  kPending,
};

struct PendingSafeCommand {
  SafeCommandParseResult command = {};
  SafeCommandQueueState state = SafeCommandQueueState::kIdle;
};

bool isSafeCommandId(uint8_t command_id);
SafeCommandParseResult parseSafeBleCommand(const uint8_t* data, size_t length);
bool safeCommandQueueStage(PendingSafeCommand& queue,
                           const SafeCommandParseResult& command);
bool safeCommandQueueDequeue(PendingSafeCommand& queue,
                             SafeCommandParseResult& command);
void safeCommandQueueFinish(PendingSafeCommand& queue);

constexpr uint32_t kDangerousCommandTokenTtlMs = 30000u;
constexpr uint16_t kPairingWindowDurationMinS = 1u;
constexpr uint16_t kPairingWindowDurationMaxS = 3600u;

struct DangerousCommandParams {
  uint16_t battery_scale_permille = 0;
  int16_t battery_offset_mv = 0;
  uint32_t odometer_m = 0;
  uint16_t pairing_window_duration_s = 0;
};

struct DangerousCommandParseResult {
  bool ok = false;
  CommandStatus status = CommandStatus::kOk;
  uint8_t field_id = 0;
  CommandId command_id = CommandId::kResetOdometer;
  bool has_token = false;
  uint32_t token = 0;
  DangerousCommandParams params = {};
};

struct DangerousCommandSession {
  bool active = false;
  CommandId command_id = CommandId::kResetOdometer;
  uint16_t connection_handle = 0xFFFFu;
  uint32_t issued_ms = 0;
  uint32_t token = 0;
  DangerousCommandParams params = {};
};

struct DangerousCommandHandshakeResult {
  CommandStatus status = CommandStatus::kOk;
  uint8_t field_id = 0;
  uint32_t token = 0;
  bool execute = false;
};

enum class DangerousCommandQueueState : uint8_t {
  kIdle = 0,
  kPending,
};

struct PendingDangerousCommand {
  DangerousCommandParseResult command = {};
  DangerousCommandQueueState state = DangerousCommandQueueState::kIdle;
};

bool isDangerousCommandId(uint8_t command_id);
DangerousCommandParseResult parseDangerousBleCommand(const uint8_t* data,
                                                      size_t length);
DangerousCommandHandshakeResult processDangerousCommandHandshake(
    DangerousCommandSession& session,
    const DangerousCommandParseResult& command,
    uint16_t connection_handle,
    bool bonded,
    uint32_t now_ms,
    uint32_t generated_token);
void invalidateDangerousCommandSession(DangerousCommandSession& session);
bool dangerousCommandQueueStage(PendingDangerousCommand& queue,
                                const DangerousCommandParseResult& command);
bool dangerousCommandQueueDequeue(PendingDangerousCommand& queue,
                                  DangerousCommandParseResult& command);
void dangerousCommandQueueFinish(PendingDangerousCommand& queue);

}  // namespace bike
