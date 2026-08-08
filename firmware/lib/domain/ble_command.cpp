#include "ble_command.h"

#include "protocol_codec.h"

namespace bike {

bool isSafeCommandId(uint8_t command_id) {
  return command_id >= static_cast<uint8_t>(CommandId::kResetTrip) &&
         command_id <= static_cast<uint8_t>(CommandId::kGetDiagnostic);
}

namespace {

uint16_t readPayloadU16(const uint8_t* payload) {
  return static_cast<uint16_t>(payload[0]) |
         static_cast<uint16_t>(payload[1]) << 8u;
}

SafeCommandParseResult fail(CommandStatus status, CommandFieldId field) {
  SafeCommandParseResult result;
  result.status = status;
  result.field_id = static_cast<uint8_t>(field);
  return result;
}

bool validateNoPayload(const CommandPacket& packet, SafeCommandParseResult& result) {
  if (packet.payload_len != 0) {
    result = fail(CommandStatus::kErrLength, CommandFieldId::kPayloadLen);
    return false;
  }
  return true;
}

}  // namespace

SafeCommandParseResult parseSafeBleCommand(const uint8_t* data, size_t length) {
  SafeCommandParseResult result;

  if (data == nullptr || length < kCommandMinSize || length > kCommandMaxSize) {
    return fail(CommandStatus::kErrLength, CommandFieldId::kPayloadLen);
  }

  if (data[0] != kBleStructVersion) {
    return fail(CommandStatus::kErrStructVersion, CommandFieldId::kStructVersion);
  }

  CommandPacket packet = {};
  if (!decodeCommand(data, length, packet)) {
    return fail(CommandStatus::kErrLength, CommandFieldId::kPayloadLen);
  }

  result.command_id = static_cast<CommandId>(packet.command_id);

  if (!isSafeCommandId(packet.command_id)) {
    return fail(CommandStatus::kErrUnknownCommand, CommandFieldId::kCommandId);
  }

  if (packet.flags != 0) {
    return fail(CommandStatus::kErrRange, CommandFieldId::kFlags);
  }

  switch (result.command_id) {
    case CommandId::kResetTrip:
    case CommandId::kResetMaxSpeed:
    case CommandId::kForceSave:
    case CommandId::kDisplayOn:
    case CommandId::kDisplayOff:
    case CommandId::kSensorTestStop:
    case CommandId::kBatteryTest:
    case CommandId::kStartDiagnostic:
    case CommandId::kGetDiagnostic:
      if (!validateNoPayload(packet, result)) return result;
      break;

    case CommandId::kDisplayTest:
      if (packet.payload_len != 1) {
        return fail(CommandStatus::kErrLength, CommandFieldId::kPayloadLen);
      }
      result.params.display_test_pattern = packet.payload[0];
      if (result.params.display_test_pattern > kDisplayTestPatternMax) {
        return fail(CommandStatus::kErrRange, CommandFieldId::kPayload);
      }
      break;

    case CommandId::kSensorTestStart:
      if (packet.payload_len != 2) {
        return fail(CommandStatus::kErrLength, CommandFieldId::kPayloadLen);
      }
      result.params.sensor_test_duration_s = readPayloadU16(packet.payload);
      if (result.params.sensor_test_duration_s < kSensorTestDurationMinS ||
          result.params.sensor_test_duration_s > kSensorTestDurationMaxS) {
        return fail(CommandStatus::kErrRange, CommandFieldId::kPayload);
      }
      break;

    default:
      return fail(CommandStatus::kErrUnknownCommand, CommandFieldId::kCommandId);
  }

  result.ok = true;
  return result;
}

bool safeCommandQueueStage(PendingSafeCommand& queue,
                           const SafeCommandParseResult& command) {
  if (queue.state != SafeCommandQueueState::kIdle || !command.ok) {
    return false;
  }
  queue.command = command;
  queue.state = SafeCommandQueueState::kPending;
  return true;
}

bool safeCommandQueueDequeue(PendingSafeCommand& queue,
                             SafeCommandParseResult& command) {
  if (queue.state != SafeCommandQueueState::kPending) {
    return false;
  }
  command = queue.command;
  return true;
}

void safeCommandQueueFinish(PendingSafeCommand& queue) {
  queue.state = SafeCommandQueueState::kIdle;
}

bool isDangerousCommandId(uint8_t command_id) {
  switch (static_cast<CommandId>(command_id)) {
    case CommandId::kResetOdometer:
    case CommandId::kFactoryReset:
    case CommandId::kReboot:
    case CommandId::kSetBatteryCal:
    case CommandId::kSetOdometer:
    case CommandId::kOpenPairingWindow:
      return true;
    default:
      return false;
  }
}

namespace {

uint32_t readCommandU32(const uint8_t* payload) {
  return static_cast<uint32_t>(payload[0]) |
         static_cast<uint32_t>(payload[1]) << 8u |
         static_cast<uint32_t>(payload[2]) << 16u |
         static_cast<uint32_t>(payload[3]) << 24u;
}

DangerousCommandParseResult failDangerous(CommandStatus status,
                                           CommandFieldId field,
                                           CommandId command_id =
                                               CommandId::kResetOdometer) {
  DangerousCommandParseResult result;
  result.status = status;
  result.field_id = static_cast<uint8_t>(field);
  result.command_id = command_id;
  return result;
}

size_t dangerousParameterLength(CommandId command_id) {
  switch (command_id) {
    case CommandId::kSetBatteryCal:
    case CommandId::kSetOdometer:
      return 4;
    case CommandId::kOpenPairingWindow:
      return 2;
    default:
      return 0;
  }
}

bool dangerousParamsEqual(const DangerousCommandParams& left,
                          const DangerousCommandParams& right) {
  return left.battery_scale_permille == right.battery_scale_permille &&
         left.battery_offset_mv == right.battery_offset_mv &&
         left.odometer_m == right.odometer_m &&
         left.pairing_window_duration_s == right.pairing_window_duration_s;
}

}  // namespace

DangerousCommandParseResult parseDangerousBleCommand(const uint8_t* data,
                                                      size_t length) {
  if (data == nullptr || length < kCommandMinSize || length > kCommandMaxSize) {
    return failDangerous(CommandStatus::kErrLength,
                         CommandFieldId::kPayloadLen);
  }
  if (data[0] != kBleStructVersion) {
    return failDangerous(CommandStatus::kErrStructVersion,
                         CommandFieldId::kStructVersion);
  }

  CommandPacket packet = {};
  if (!decodeCommand(data, length, packet)) {
    return failDangerous(CommandStatus::kErrLength,
                         CommandFieldId::kPayloadLen);
  }
  const CommandId command_id = static_cast<CommandId>(packet.command_id);
  if (!isDangerousCommandId(packet.command_id)) {
    return failDangerous(CommandStatus::kErrUnknownCommand,
                         CommandFieldId::kCommandId, command_id);
  }
  if ((packet.flags & ~kCommandFlagHasToken) != 0) {
    return failDangerous(CommandStatus::kErrRange, CommandFieldId::kFlags,
                         command_id);
  }

  DangerousCommandParseResult result;
  result.command_id = command_id;
  result.has_token = (packet.flags & kCommandFlagHasToken) != 0;
  const size_t parameter_length = dangerousParameterLength(command_id);
  const size_t expected_length =
      parameter_length + (result.has_token ? sizeof(uint32_t) : 0u);
  if (packet.payload_len != expected_length) {
    return failDangerous(CommandStatus::kErrLength,
                         CommandFieldId::kPayloadLen, command_id);
  }

  switch (command_id) {
    case CommandId::kSetBatteryCal:
      result.params.battery_scale_permille = readPayloadU16(packet.payload);
      result.params.battery_offset_mv =
          static_cast<int16_t>(readPayloadU16(packet.payload + 2));
      if (result.params.battery_scale_permille < 800u ||
          result.params.battery_scale_permille > 1200u ||
          result.params.battery_offset_mv < -500 ||
          result.params.battery_offset_mv > 500) {
        return failDangerous(CommandStatus::kErrRange,
                             CommandFieldId::kPayload, command_id);
      }
      break;
    case CommandId::kSetOdometer:
      result.params.odometer_m = readCommandU32(packet.payload);
      break;
    case CommandId::kOpenPairingWindow:
      result.params.pairing_window_duration_s = readPayloadU16(packet.payload);
      if (result.params.pairing_window_duration_s <
              kPairingWindowDurationMinS ||
          result.params.pairing_window_duration_s >
              kPairingWindowDurationMaxS) {
        return failDangerous(CommandStatus::kErrRange,
                             CommandFieldId::kPayload, command_id);
      }
      break;
    default:
      break;
  }

  if (result.has_token) {
    result.token = readCommandU32(packet.payload + parameter_length);
  }
  result.ok = true;
  return result;
}

void invalidateDangerousCommandSession(DangerousCommandSession& session) {
  session = DangerousCommandSession{};
}

DangerousCommandHandshakeResult processDangerousCommandHandshake(
    DangerousCommandSession& session,
    const DangerousCommandParseResult& command,
    uint16_t connection_handle,
    bool bonded,
    uint32_t now_ms,
    uint32_t generated_token) {
  DangerousCommandHandshakeResult result;
  if (!command.ok) {
    result.status = command.status;
    result.field_id = command.field_id;
    return result;
  }
  if (!bonded) {
    invalidateDangerousCommandSession(session);
    result.status = CommandStatus::kErrNotPaired;
    return result;
  }

  if (!command.has_token) {
    if (generated_token == 0) {
      result.status = CommandStatus::kErrBusy;
      return result;
    }
    session.active = true;
    session.command_id = command.command_id;
    session.connection_handle = connection_handle;
    session.issued_ms = now_ms;
    session.token = generated_token;
    session.params = command.params;
    result.status = CommandStatus::kNeedsConfirm;
    result.token = generated_token;
    return result;
  }

  if (!session.active) {
    result.status = CommandStatus::kErrTokenInvalid;
    return result;
  }
  if (session.command_id != command.command_id ||
      session.connection_handle != connection_handle ||
      !dangerousParamsEqual(session.params, command.params)) {
    invalidateDangerousCommandSession(session);
    result.status = CommandStatus::kErrTokenInvalid;
    return result;
  }
  if (static_cast<uint32_t>(now_ms - session.issued_ms) >=
      kDangerousCommandTokenTtlMs) {
    invalidateDangerousCommandSession(session);
    result.status = CommandStatus::kErrTokenExpired;
    return result;
  }
  if (session.token != command.token) {
    invalidateDangerousCommandSession(session);
    result.status = CommandStatus::kErrTokenInvalid;
    return result;
  }

  invalidateDangerousCommandSession(session);
  result.execute = true;
  return result;
}

bool dangerousCommandQueueStage(PendingDangerousCommand& queue,
                                const DangerousCommandParseResult& command) {
  if (queue.state != DangerousCommandQueueState::kIdle || !command.ok ||
      !command.has_token) {
    return false;
  }
  queue.command = command;
  queue.state = DangerousCommandQueueState::kPending;
  return true;
}

bool dangerousCommandQueueDequeue(PendingDangerousCommand& queue,
                                  DangerousCommandParseResult& command) {
  if (queue.state != DangerousCommandQueueState::kPending) return false;
  command = queue.command;
  return true;
}

void dangerousCommandQueueFinish(PendingDangerousCommand& queue) {
  queue.state = DangerousCommandQueueState::kIdle;
}

}  // namespace bike
