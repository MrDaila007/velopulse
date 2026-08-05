#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config_codec.h"

namespace bike {

// Parses a 96-char lowercase/uppercase hex wire_v1 payload (48 bytes).
// Accumulates a 96-char wire_v1 hex line after a bare `load-config` command.
class PendingWireV1Reader {
 public:
  void begin();
  bool active() const { return active_; }
  bool feed(char ch);
  bool ready() const;
  const char* line() const;
  void reset();

 private:
  char buffer_[97] = {};
  size_t length_ = 0;
  bool active_ = false;
};

bool parseWireV1Hex(const char* hex, uint8_t output[kDeviceConfigPayloadSize]);

// Parses a non-negative decimal uint64. Returns false on empty/invalid input.
bool parseUint64Decimal(const char* text, uint64_t& out);

}  // namespace bike
