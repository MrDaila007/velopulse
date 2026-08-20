#include "protocol_codec.h"

#include <string.h>

namespace bike {
namespace {

void writeU16(uint8_t* output, uint16_t value) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8u);
}

void writeU32(uint8_t* output, uint32_t value) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8u);
  output[2] = static_cast<uint8_t>(value >> 16u);
  output[3] = static_cast<uint8_t>(value >> 24u);
}

uint16_t readU16(const uint8_t* input) {
  return static_cast<uint16_t>(input[0]) |
         static_cast<uint16_t>(input[1]) << 8u;
}

uint32_t readU32(const uint8_t* input) {
  return static_cast<uint32_t>(input[0]) |
         static_cast<uint32_t>(input[1]) << 8u |
         static_cast<uint32_t>(input[2]) << 16u |
         static_cast<uint32_t>(input[3]) << 24u;
}

}  // namespace

void encodeDeviceInfo(const DeviceInfoPacket& info,
                      uint8_t output[kDeviceInfoSize]) {
  memset(output, 0, kDeviceInfoSize);
  output[0] = info.struct_version;
  output[1] = info.proto_major;
  output[2] = info.proto_minor;
  output[3] = info.hw_revision;
  memcpy(output + 4, info.model, 16);
  memcpy(output + 20, info.fw_version, 12);
  memcpy(output + 32, info.serial, 8);
  writeU32(output + 40, info.uptime_s);
  output[44] = info.reset_reason;
  writeU16(output + 45, info.boot_count);
  output[47] = info.flags;
}

bool decodeDeviceInfo(const uint8_t* input,
                      size_t length,
                      DeviceInfoPacket& info) {
  if (input == nullptr || length != kDeviceInfoSize ||
      input[0] != kBleStructVersion) {
    return false;
  }

  DeviceInfoPacket decoded = {};
  decoded.struct_version = input[0];
  decoded.proto_major = input[1];
  decoded.proto_minor = input[2];
  decoded.hw_revision = input[3];
  memcpy(decoded.model, input + 4, 16);
  memcpy(decoded.fw_version, input + 20, 12);
  memcpy(decoded.serial, input + 32, 8);
  decoded.uptime_s = readU32(input + 40);
  decoded.reset_reason = input[44];
  decoded.boot_count = readU16(input + 45);
  decoded.flags = input[47];
  info = decoded;
  return true;
}

void encodeTelemetry(const TelemetryPacket& telemetry,
                     uint8_t output[kTelemetrySize]) {
  memset(output, 0, kTelemetrySize);
  output[0] = telemetry.struct_version;
  output[1] = telemetry.flags;
  writeU16(output + 2, telemetry.speed_x100);
  writeU16(output + 4, telemetry.avg_speed_x100);
  writeU16(output + 6, telemetry.max_speed_x100);
  writeU32(output + 8, telemetry.trip_distance_cm);
  writeU32(output + 12, telemetry.moving_time_s);
  writeU32(output + 16, telemetry.odometer_m);
  writeU16(output + 20, telemetry.battery_mv);
  output[22] = telemetry.battery_pct;
  output[23] = telemetry.ride_state;
  writeU32(output + 24, telemetry.revolutions);
  writeU32(output + 28, telemetry.last_pulse_age_ms);
  writeU16(output + 32, telemetry.seq);
  output[34] = telemetry.sensor_state;
  output[35] = telemetry.power_state;
  if (telemetry.struct_version >= kTelemetryStructVersion) {
    writeU16(output + 36, telemetry.cadence_x10);
    output[38] = telemetry.csc_flags;
    output[39] = telemetry.reserved_csc;
    writeU32(output + 40, telemetry.last_crank_event_age_ms);
  }
}

bool decodeTelemetry(const uint8_t* input,
                     size_t length,
                     TelemetryPacket& telemetry) {
  if (input == nullptr || length < kTelemetryV1Size || input[0] < 1u) {
    return false;
  }

  TelemetryPacket decoded = {};
  decoded.struct_version = input[0];
  decoded.flags = input[1];
  decoded.speed_x100 = readU16(input + 2);
  decoded.avg_speed_x100 = readU16(input + 4);
  decoded.max_speed_x100 = readU16(input + 6);
  decoded.trip_distance_cm = readU32(input + 8);
  decoded.moving_time_s = readU32(input + 12);
  decoded.odometer_m = readU32(input + 16);
  decoded.battery_mv = readU16(input + 20);
  decoded.battery_pct = input[22];
  decoded.ride_state = input[23];
  decoded.revolutions = readU32(input + 24);
  decoded.last_pulse_age_ms = readU32(input + 28);
  decoded.seq = readU16(input + 32);
  decoded.sensor_state = input[34];
  decoded.power_state = input[35];
  decoded.last_crank_event_age_ms = kTelemetryNoPulseAgeMs;
  if (length >= kTelemetrySize) {
    decoded.cadence_x10 = readU16(input + 36);
    decoded.csc_flags = input[38];
    decoded.reserved_csc = input[39];
    decoded.last_crank_event_age_ms = readU32(input + 40);
  }
  telemetry = decoded;
  return true;
}

size_t encodeCommand(const CommandPacket& command,
                     uint8_t* output,
                     size_t capacity) {
  if (output == nullptr || command.payload_len > kCommandMaxPayload ||
      capacity < kCommandHeaderSize + command.payload_len) {
    return 0;
  }
  output[0] = command.struct_version;
  output[1] = command.command_id;
  output[2] = command.flags;
  output[3] = command.payload_len;
  if (command.payload_len > 0) {
    memcpy(output + 4, command.payload, command.payload_len);
  }
  return kCommandHeaderSize + command.payload_len;
}

bool decodeCommand(const uint8_t* input,
                   size_t length,
                   CommandPacket& command) {
  if (input == nullptr || length < kCommandMinSize ||
      length > kCommandMaxSize || input[0] != kBleStructVersion) {
    return false;
  }
  const uint8_t payload_len = input[3];
  if (payload_len > kCommandMaxPayload ||
      length != kCommandHeaderSize + payload_len) {
    return false;
  }

  CommandPacket decoded = {};
  decoded.struct_version = input[0];
  decoded.command_id = input[1];
  decoded.flags = input[2];
  decoded.payload_len = payload_len;
  if (payload_len > 0) {
    memcpy(decoded.payload, input + 4, payload_len);
  }
  command = decoded;
  return true;
}

size_t encodeCommandResult(const CommandResultPacket& result,
                           uint8_t* output,
                           size_t capacity) {
  if (output == nullptr || result.payload_len > kCommandResultMaxPayload ||
      capacity < kCommandResultHeaderSize + result.payload_len) {
    return 0;
  }
  output[0] = result.struct_version;
  output[1] = result.command_id;
  output[2] = result.status;
  output[3] = result.detail;
  writeU32(output + 4, result.token);
  output[8] = result.payload_len;
  if (result.payload_len > 0) {
    memcpy(output + 9, result.payload, result.payload_len);
  }
  return kCommandResultHeaderSize + result.payload_len;
}

bool decodeCommandResult(const uint8_t* input,
                         size_t length,
                         CommandResultPacket& result) {
  if (input == nullptr || length < kCommandResultMinSize ||
      length > kCommandResultMaxSize || input[0] != kBleStructVersion) {
    return false;
  }
  const uint8_t payload_len = input[8];
  if (payload_len > kCommandResultMaxPayload ||
      length != kCommandResultHeaderSize + payload_len) {
    return false;
  }

  CommandResultPacket decoded = {};
  decoded.struct_version = input[0];
  decoded.command_id = input[1];
  decoded.status = input[2];
  decoded.detail = input[3];
  decoded.token = readU32(input + 4);
  decoded.payload_len = payload_len;
  if (payload_len > 0) {
    memcpy(decoded.payload, input + 9, payload_len);
  }
  result = decoded;
  return true;
}

size_t encodeErrorLog(const ErrorLogPacket& log,
                      uint8_t* output,
                      size_t capacity) {
  if (output == nullptr || log.entry_count < 1 ||
      log.entry_count > kErrorLogMaxEntries) {
    return 0;
  }
  const size_t encoded =
      kErrorLogHeaderSize +
      static_cast<size_t>(log.entry_count) * kErrorLogEntrySize;
  if (capacity < encoded) {
    return 0;
  }
  output[0] = log.struct_version;
  output[1] = log.entry_count;
  for (uint8_t i = 0; i < log.entry_count; ++i) {
    uint8_t* entry = output + kErrorLogHeaderSize + i * kErrorLogEntrySize;
    writeU32(entry, log.entries[i].uptime_s);
    entry[4] = log.entries[i].code;
    entry[5] = log.entries[i].severity;
    writeU16(entry + 6, log.entries[i].detail);
  }
  return encoded;
}

bool decodeErrorLog(const uint8_t* input,
                    size_t length,
                    ErrorLogPacket& log) {
  if (input == nullptr || length < kErrorLogMinSize ||
      length > kErrorLogMaxSize || input[0] != kBleStructVersion) {
    return false;
  }
  const uint8_t entry_count = input[1];
  if (entry_count < 1 || entry_count > kErrorLogMaxEntries) {
    return false;
  }
  const size_t expected =
      kErrorLogHeaderSize +
      static_cast<size_t>(entry_count) * kErrorLogEntrySize;
  if (length != expected) {
    return false;
  }

  ErrorLogPacket decoded = {};
  decoded.struct_version = input[0];
  decoded.entry_count = entry_count;
  for (uint8_t i = 0; i < entry_count; ++i) {
    const uint8_t* entry =
        input + kErrorLogHeaderSize + i * kErrorLogEntrySize;
    decoded.entries[i].uptime_s = readU32(entry);
    decoded.entries[i].code = entry[4];
    decoded.entries[i].severity = entry[5];
    decoded.entries[i].detail = readU16(entry + 6);
  }
  log = decoded;
  return true;
}

}  // namespace bike
