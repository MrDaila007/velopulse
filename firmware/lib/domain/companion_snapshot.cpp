#include "companion_snapshot.h"

#include <stdio.h>
#include <string.h>

namespace bike {
namespace {

uint16_t readU16(const uint8_t* input) {
  return static_cast<uint16_t>(input[0]) |
         static_cast<uint16_t>(input[1]) << 8u;
}

uint32_t readU32(const uint8_t* input) {
  return static_cast<uint32_t>(input[0]) |
         static_cast<uint32_t>(input[1]) << 8u |
         static_cast<uint32_t>(input[2]) << 16u |
         static_cast<uint32_t>(input[3]) << 24u;
}

int16_t readI16(const uint8_t* input) {
  return static_cast<int16_t>(readU16(input));
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

void writeI16(uint8_t* output, int16_t value) {
  writeU16(output, static_cast<uint16_t>(value));
}

void formatCompanionWeather(char* temp,
                            size_t temp_capacity,
                            char* rain,
                            size_t rain_capacity,
                            const CompanionSnapshotPacket& packet,
                            bool stale) {
  if (temp != nullptr && temp_capacity > 0) temp[0] = '\0';
  if (rain != nullptr && rain_capacity > 0) rain[0] = '\0';
  if ((packet.flags & kCompanionFlagWeatherValid) == 0) return;

  if (temp != nullptr && temp_capacity > 0 &&
      packet.temp_c_x10 != kCompanionTempInvalid) {
    const int16_t whole = packet.temp_c_x10 / 10;
    const int16_t frac = packet.temp_c_x10 >= 0
                             ? packet.temp_c_x10 % 10
                             : -((-packet.temp_c_x10) % 10);
    if (frac == 0) {
      snprintf(temp, temp_capacity, "%s%dC", whole >= 0 ? "+" : "", whole);
    } else {
      snprintf(temp, temp_capacity, "%s%d.%dC", whole >= 0 ? "+" : "", whole,
               frac);
    }
    if (stale) {
      const size_t used = strlen(temp);
      if (used + 1u < temp_capacity) {
        temp[used] = '?';
        temp[used + 1u] = '\0';
      }
    }
  }

  if (rain != nullptr && rain_capacity > 0) {
    if (packet.pop_pct != kCompanionPopInvalid) {
      snprintf(rain, rain_capacity, "R%u%%", packet.pop_pct);
    } else if ((packet.flags & kCompanionFlagRainNow) != 0) {
      snprintf(rain, rain_capacity, "RAIN");
    }
    if (rain[0] != '\0' && stale) {
      const size_t used = strlen(rain);
      if (used + 1u < rain_capacity) {
        rain[used] = '?';
        rain[used + 1u] = '\0';
      }
    }
  }
}

}  // namespace

bool decodeCompanionSnapshot(const uint8_t* input,
                             size_t length,
                             CompanionSnapshotPacket& out) {
  if (input == nullptr || length < kCompanionSnapshotSize || input[0] != 1u) {
    return false;
  }
  out = {};
  out.struct_version = input[0];
  out.unix_time = readU32(input + 1);
  out.tz_offset_min = readI16(input + 5);
  out.temp_c_x10 = readI16(input + 7);
  out.pop_pct = input[9];
  out.flags = input[10];
  out.valid_until = readU32(input + 11);
  return true;
}

void encodeCompanionSnapshot(const CompanionSnapshotPacket& packet,
                             uint8_t output[kCompanionSnapshotSize]) {
  memset(output, 0, kCompanionSnapshotSize);
  output[0] = packet.struct_version;
  writeU32(output + 1, packet.unix_time);
  writeI16(output + 5, packet.tz_offset_min);
  writeI16(output + 7, packet.temp_c_x10);
  output[9] = packet.pop_pct;
  output[10] = packet.flags;
  writeU32(output + 11, packet.valid_until);
}

void CompanionState::apply(const CompanionSnapshotPacket& packet,
                           uint32_t now_ms) {
  packet_ = packet;
  sync_mono_ms_ = now_ms;
  synced_ = true;
}

void CompanionState::clear() {
  packet_ = {};
  sync_mono_ms_ = 0;
  synced_ = false;
}

uint32_t CompanionState::localUnix(uint32_t now_ms) const {
  if (!synced_ || (packet_.flags & kCompanionFlagTimeValid) == 0) return 0;
  const uint32_t elapsed_s = (now_ms - sync_mono_ms_) / 1000u;
  return packet_.unix_time + elapsed_s;
}

bool CompanionState::isStale(uint32_t now_ms) const {
  if (!synced_) return true;
  if ((packet_.flags & kCompanionFlagStale) != 0) return true;
  if (packet_.valid_until == 0) return false;
  return localUnix(now_ms) > packet_.valid_until;
}

CompanionHeaderView CompanionState::header(uint32_t now_ms) const {
  CompanionHeaderView view = {};
  if (!synced_ || (packet_.flags & kCompanionFlagTimeValid) == 0) {
    return view;
  }

  const bool stale = isStale(now_ms);
  const uint32_t local_s =
      localUnix(now_ms) + static_cast<uint32_t>(packet_.tz_offset_min) * 60u;
  const uint8_t hour = static_cast<uint8_t>((local_s / 3600u) % 24u);
  const uint8_t minute = static_cast<uint8_t>((local_s / 60u) % 60u);

  if (stale) {
    snprintf(view.text, sizeof(view.text), "%02u:%02u?", hour, minute);
  } else {
    snprintf(view.text, sizeof(view.text), "%02u:%02u", hour, minute);
  }
  view.valid = true;
  view.stale = stale;
  return view;
}

CompanionWeatherView CompanionState::weather(uint32_t now_ms) const {
  CompanionWeatherView view = {};
  if (!synced_ || (packet_.flags & kCompanionFlagWeatherValid) == 0) {
    return view;
  }
  const bool stale = isStale(now_ms);
  formatCompanionWeather(view.temp, sizeof(view.temp), view.rain,
                         sizeof(view.rain), packet_, stale);
  if (view.temp[0] == '\0' && view.rain[0] == '\0') return view;
  view.valid = true;
  view.stale = stale;
  return view;
}

}  // namespace bike
