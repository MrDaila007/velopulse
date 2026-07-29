#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ble_protocol.h"
#include "config.h"
#include "config_codec.h"

namespace bike {

// Canonical little-endian codecs for BLE wire structures (protocol v1.0).
// Configuration reuses DeviceConfig + config_codec — no second layout.

void encodeDeviceInfo(const DeviceInfoPacket& info,
                      uint8_t output[kDeviceInfoSize]);
bool decodeDeviceInfo(const uint8_t* input,
                      size_t length,
                      DeviceInfoPacket& info);

void encodeTelemetry(const TelemetryPacket& telemetry,
                     uint8_t output[kTelemetrySize]);
bool decodeTelemetry(const uint8_t* input,
                     size_t length,
                     TelemetryPacket& telemetry);

// Thin wrappers over config_codec (DeviceConfig is the domain model).
inline void encodeConfiguration(const DeviceConfig& config,
                                uint8_t output[kConfigurationSize]) {
  encodeDeviceConfig(config, output);
}
inline bool decodeConfiguration(const uint8_t* input,
                                size_t length,
                                DeviceConfig& config) {
  return decodeDeviceConfig(input, length, config);
}

// Returns encoded length on success (4..20), 0 on failure.
size_t encodeCommand(const CommandPacket& command,
                     uint8_t* output,
                     size_t capacity);
bool decodeCommand(const uint8_t* input,
                   size_t length,
                   CommandPacket& command);

// Returns encoded length on success (9..25), 0 on failure.
size_t encodeCommandResult(const CommandResultPacket& result,
                           uint8_t* output,
                           size_t capacity);
bool decodeCommandResult(const uint8_t* input,
                         size_t length,
                         CommandResultPacket& result);

// Returns encoded length on success (10..34), 0 on failure.
size_t encodeErrorLog(const ErrorLogPacket& log,
                      uint8_t* output,
                      size_t capacity);
bool decodeErrorLog(const uint8_t* input,
                    size_t length,
                    ErrorLogPacket& log);

}  // namespace bike
