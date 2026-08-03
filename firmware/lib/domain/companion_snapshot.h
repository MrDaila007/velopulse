#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bike {

constexpr size_t kCompanionSnapshotSize = 15;
constexpr int16_t kCompanionTempInvalid = 0x7FFF;
constexpr uint8_t kCompanionPopInvalid = 0xFF;

constexpr uint8_t kCompanionFlagTimeValid = 1u << 0;
constexpr uint8_t kCompanionFlagWeatherValid = 1u << 1;
constexpr uint8_t kCompanionFlagRainNow = 1u << 2;
constexpr uint8_t kCompanionFlagRainSoon = 1u << 3;
constexpr uint8_t kCompanionFlagStale = 1u << 4;

struct CompanionSnapshotPacket {
  uint8_t struct_version = 1;
  uint32_t unix_time = 0;
  int16_t tz_offset_min = 0;
  int16_t temp_c_x10 = kCompanionTempInvalid;
  uint8_t pop_pct = kCompanionPopInvalid;
  uint8_t flags = 0;
  uint32_t valid_until = 0;
};

struct CompanionHeaderView {
  char text[20] = {};
  bool valid = false;
  bool stale = false;
};

struct CompanionWeatherView {
  char temp[10] = {};
  char rain[8] = {};
  bool valid = false;
  bool stale = false;
};

bool decodeCompanionSnapshot(const uint8_t* input,
                             size_t length,
                             CompanionSnapshotPacket& out);
void encodeCompanionSnapshot(const CompanionSnapshotPacket& packet,
                             uint8_t output[kCompanionSnapshotSize]);

class CompanionState {
 public:
  void apply(const CompanionSnapshotPacket& packet, uint32_t now_ms);
  void clear();

  uint32_t localUnix(uint32_t now_ms) const;
  bool isStale(uint32_t now_ms) const;
  CompanionHeaderView header(uint32_t now_ms) const;
  CompanionWeatherView weather(uint32_t now_ms) const;

 private:
  CompanionSnapshotPacket packet_ = {};
  uint32_t sync_mono_ms_ = 0;
  bool synced_ = false;
};

}  // namespace bike
