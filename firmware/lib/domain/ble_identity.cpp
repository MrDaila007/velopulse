#include "ble_identity.h"

#include <string.h>

namespace bike {
namespace {

char hexNibble(uint8_t value) {
  return static_cast<char>(value < 10 ? ('0' + value) : ('A' + (value - 10)));
}

}  // namespace

bool isDeviceNamePlaceholder(const char* name) {
  if (name == nullptr || name[0] == '\0') return true;
  return strcmp(name, kDeviceNamePlaceholder) == 0;
}

void formatDeviceNameWithSerial(uint16_t serial_suffix_hex,
                                char* output,
                                size_t capacity) {
  if (output == nullptr || capacity == 0) return;
  // "BikeComp-" (9) + 4 hex + NUL
  if (capacity < 14) {
    output[0] = '\0';
    return;
  }
  memcpy(output, "BikeComp-", 9);
  output[9] = hexNibble(static_cast<uint8_t>((serial_suffix_hex >> 12) & 0xFu));
  output[10] = hexNibble(static_cast<uint8_t>((serial_suffix_hex >> 8) & 0xFu));
  output[11] = hexNibble(static_cast<uint8_t>((serial_suffix_hex >> 4) & 0xFu));
  output[12] = hexNibble(static_cast<uint8_t>(serial_suffix_hex & 0xFu));
  output[13] = '\0';
}

void resolveDeviceLocalName(const char* configured_name,
                            uint16_t serial_suffix_hex,
                            char* output,
                            size_t capacity) {
  if (output == nullptr || capacity == 0) return;
  if (!isDeviceNamePlaceholder(configured_name)) {
    strncpy(output, configured_name, capacity - 1u);
    output[capacity - 1u] = '\0';
    return;
  }
  formatDeviceNameWithSerial(serial_suffix_hex, output, capacity);
}

}  // namespace bike
