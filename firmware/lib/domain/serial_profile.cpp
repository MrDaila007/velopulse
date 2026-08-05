#include "serial_profile.h"

#include <ctype.h>

namespace bike {
namespace {

bool hexNibble(char value, uint8_t& out) {
  if (value >= '0' && value <= '9') {
    out = static_cast<uint8_t>(value - '0');
    return true;
  }
  const char lower = static_cast<char>(tolower(static_cast<unsigned char>(value)));
  if (lower >= 'a' && lower <= 'f') {
    out = static_cast<uint8_t>(10 + lower - 'a');
    return true;
  }
  return false;
}

}  // namespace

void PendingWireV1Reader::begin() {
  length_ = 0;
  buffer_[0] = '\0';
  active_ = true;
}

bool PendingWireV1Reader::feed(char ch) {
  if (!active_) return false;
  if (ch == '\r' || ch == '\n') {
    buffer_[length_] = '\0';
    active_ = false;
    return length_ == kDeviceConfigPayloadSize * 2;
  }
  if (length_ < kDeviceConfigPayloadSize * 2) {
    buffer_[length_++] = ch;
    buffer_[length_] = '\0';
  }
  return false;
}

bool PendingWireV1Reader::ready() const {
  return !active_ && length_ == kDeviceConfigPayloadSize * 2;
}

const char* PendingWireV1Reader::line() const { return buffer_; }

void PendingWireV1Reader::reset() {
  length_ = 0;
  buffer_[0] = '\0';
  active_ = false;
}

bool parseWireV1Hex(const char* hex, uint8_t output[kDeviceConfigPayloadSize]) {
  if (hex == nullptr || output == nullptr) return false;
  for (size_t i = 0; i < kDeviceConfigPayloadSize; ++i) {
    const char high = hex[i * 2];
    const char low = hex[i * 2 + 1];
    if (high == '\0' || low == '\0') return false;
    uint8_t high_nibble = 0;
    uint8_t low_nibble = 0;
    if (!hexNibble(high, high_nibble) || !hexNibble(low, low_nibble)) {
      return false;
    }
    output[i] = static_cast<uint8_t>((high_nibble << 4u) | low_nibble);
  }
  if (hex[kDeviceConfigPayloadSize * 2] != '\0') return false;
  return true;
}

bool parseUint64Decimal(const char* text, uint64_t& out) {
  if (text == nullptr) return false;
  while (*text == ' ' || *text == '\t') ++text;
  if (*text == '\0') return false;

  uint64_t value = 0;
  for (; *text != '\0'; ++text) {
    if (*text == ' ' || *text == '\t') {
      while (*text == ' ' || *text == '\t') ++text;
      return *text == '\0' ? (out = value, true) : false;
    }
    if (*text < '0' || *text > '9') return false;
    const uint64_t digit = static_cast<uint64_t>(*text - '0');
    if (value > (UINT64_MAX - digit) / 10u) return false;
    value = value * 10u + digit;
  }
  out = value;
  return true;
}

}  // namespace bike
