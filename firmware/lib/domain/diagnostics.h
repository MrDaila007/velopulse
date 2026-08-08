#pragma once

#include <stddef.h>
#include <stdint.h>

#include "pulse_filter.h"
#include "storage_manager.h"

namespace bike {

// GET_DIAGNOSTIC Command Result payload (§6.1), 16 bytes little-endian.
constexpr size_t kDiagnosticPayloadSize = 16;

constexpr uint8_t kSelftestDisplayOk = 1u << 0;
constexpr uint8_t kSelftestHallPinOk = 1u << 1;
constexpr uint8_t kSelftestAdcOk = 1u << 2;
constexpr uint8_t kSelftestFsOk = 1u << 3;
constexpr uint8_t kSelftestConfigValid = 1u << 4;
constexpr uint8_t kSelftestBleOk = 1u << 5;
constexpr uint8_t kSelftestWatchdogOk = 1u << 6;

struct DiagnosticSnapshot {
  uint32_t raw_pulse_count = 0;
  uint16_t rejected_debounce = 0;
  uint16_t rejected_overspeed = 0;
  uint16_t isr_overflow = 0;
  uint16_t flash_write_count = 0;
  uint16_t free_heap_units = 0;  // free_heap_bytes / 16
  uint8_t i2c_error_count = 0;
  uint8_t selftest_mask = 0;
};

struct DiagnosticSources {
  uint32_t raw_pulse_count = 0;
  uint32_t rejected_debounce = 0;
  uint32_t rejected_overspeed = 0;
  uint32_t isr_overflow = 0;
  uint32_t flash_write_count = 0;
  uint32_t free_heap_bytes = 0;
  uint8_t i2c_error_count = 0;
  uint8_t selftest_mask = 0;
};

DiagnosticSources makeDiagnosticSources(const StorageCounters& storage,
                                        const PulseFilterCounters& pulses,
                                        uint32_t raw_pulse_count,
                                        uint32_t isr_overflow,
                                        uint32_t free_heap_bytes,
                                        uint8_t i2c_error_count,
                                        uint8_t selftest_mask);

DiagnosticSnapshot buildDiagnosticSnapshot(const DiagnosticSources& sources);
void encodeDiagnosticPayload(const DiagnosticSnapshot& snapshot,
                             uint8_t output[kDiagnosticPayloadSize]);

}  // namespace bike
