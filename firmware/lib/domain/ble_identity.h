#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bike {

// Factory default in DeviceConfig / protocol is the literal placeholder
// "BikeComp-XXXX"; advertising must substitute XXXX with 4 hex digits of the
// device serial (protocol/ble-protocol.md §3).
constexpr char kDeviceNamePlaceholder[] = "BikeComp-XXXX";

bool isDeviceNamePlaceholder(const char* name);

// Formats "BikeComp-HHHH\0" into output (needs ≥13 chars + NUL).
void formatDeviceNameWithSerial(uint16_t serial_suffix_hex,
                                char* output,
                                size_t capacity);

// Uses configured_name when it is a non-empty custom name; otherwise builds
// BikeComp-<serial> from serial_suffix_hex (low 16 bits of FICR DEVICEID[0]).
void resolveDeviceLocalName(const char* configured_name,
                            uint16_t serial_suffix_hex,
                            char* output,
                            size_t capacity);

}  // namespace bike
