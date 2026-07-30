#include "error_log.h"

namespace bike {

void ErrorLogBuffer::append(uint32_t uptime_s,
                            ErrorLogCode code,
                            ErrorLogSeverity severity,
                            uint16_t detail) {
  ErrorLogEntry& entry = entries_[next_];
  entry.uptime_s = uptime_s;
  entry.code = static_cast<uint8_t>(code);
  entry.severity = static_cast<uint8_t>(severity);
  entry.detail = detail;

  next_ = (next_ + 1u) % kErrorLogCapacity;
  if (count_ < kErrorLogCapacity) ++count_;
}

bool ErrorLogBuffer::snapshot(ErrorLogPacket& packet) const {
  if (count_ == 0) return false;

  packet = {};
  packet.struct_version = kBleStructVersion;
  const size_t output_count =
      count_ < kErrorLogMaxEntries ? count_ : kErrorLogMaxEntries;
  packet.entry_count = static_cast<uint8_t>(output_count);
  const size_t start =
      (next_ + kErrorLogCapacity - output_count) % kErrorLogCapacity;
  for (size_t i = 0; i < output_count; ++i) {
    packet.entries[i] = entries_[(start + i) % kErrorLogCapacity];
  }
  return true;
}

void ErrorLogBuffer::clear() {
  next_ = 0;
  count_ = 0;
}

}  // namespace bike
