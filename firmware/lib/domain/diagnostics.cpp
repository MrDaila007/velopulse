#include "diagnostics.h"

#include <string.h>

namespace bike {
namespace {

uint16_t saturateU16(uint32_t value) {
  return value > 0xFFFFu ? static_cast<uint16_t>(0xFFFFu)
                         : static_cast<uint16_t>(value);
}

uint8_t saturateU8(uint32_t value) {
  return value > 0xFFu ? static_cast<uint8_t>(0xFFu)
                       : static_cast<uint8_t>(value);
}

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

}  // namespace

DiagnosticSources makeDiagnosticSources(const StorageCounters& storage,
                                        const PulseFilterCounters& pulses,
                                        uint32_t raw_pulse_count,
                                        uint32_t isr_overflow,
                                        uint32_t free_heap_bytes,
                                        uint8_t i2c_error_count,
                                        uint8_t selftest_mask) {
  DiagnosticSources sources;
  sources.raw_pulse_count = raw_pulse_count;
  sources.rejected_debounce = pulses.rejected_debounce;
  sources.rejected_overspeed = pulses.rejected_overspeed;
  sources.isr_overflow = isr_overflow;
  sources.flash_write_count = storage.writes;
  sources.free_heap_bytes = free_heap_bytes;
  sources.i2c_error_count = i2c_error_count;
  sources.selftest_mask = selftest_mask;
  return sources;
}

DiagnosticSnapshot buildDiagnosticSnapshot(const DiagnosticSources& sources) {
  DiagnosticSnapshot snapshot;
  snapshot.raw_pulse_count = sources.raw_pulse_count;
  snapshot.rejected_debounce = saturateU16(sources.rejected_debounce);
  snapshot.rejected_overspeed = saturateU16(sources.rejected_overspeed);
  snapshot.isr_overflow = saturateU16(sources.isr_overflow);
  snapshot.flash_write_count = saturateU16(sources.flash_write_count);
  snapshot.free_heap_units = saturateU16(sources.free_heap_bytes / 16u);
  snapshot.i2c_error_count = saturateU8(sources.i2c_error_count);
  snapshot.selftest_mask = sources.selftest_mask;
  return snapshot;
}

void encodeDiagnosticPayload(const DiagnosticSnapshot& snapshot,
                             uint8_t output[kDiagnosticPayloadSize]) {
  memset(output, 0, kDiagnosticPayloadSize);
  writeU32(output + 0, snapshot.raw_pulse_count);
  writeU16(output + 4, snapshot.rejected_debounce);
  writeU16(output + 6, snapshot.rejected_overspeed);
  writeU16(output + 8, snapshot.isr_overflow);
  writeU16(output + 10, snapshot.flash_write_count);
  writeU16(output + 12, snapshot.free_heap_units);
  output[14] = snapshot.i2c_error_count;
  output[15] = snapshot.selftest_mask;
}

}  // namespace bike
