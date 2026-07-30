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

  if ((packet.flags & kCommandFlagHasToken) != 0) {
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

}  // namespace bike
