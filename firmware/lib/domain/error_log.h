#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ble_protocol.h"

namespace bike {

constexpr size_t kErrorLogCapacity = 16;

class ErrorLogBuffer {
 public:
  void append(uint32_t uptime_s,
              ErrorLogCode code,
              ErrorLogSeverity severity,
              uint16_t detail = 0);

  // Returns the newest protocol-sized window in chronological order.
  bool snapshot(ErrorLogPacket& packet) const;
  size_t size() const { return count_; }
  void clear();

 private:
  ErrorLogEntry entries_[kErrorLogCapacity] = {};
  size_t next_ = 0;
  size_t count_ = 0;
};

}  // namespace bike
