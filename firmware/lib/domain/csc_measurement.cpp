#include "csc_measurement.h"

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

char asciiUpper(char value) {
  if (value >= 'a' && value <= 'z') {
    return static_cast<char>(value - 'a' + 'A');
  }
  return value;
}

bool startsWithIgnoreCase(const char* value, const char* prefix) {
  if (value == nullptr || prefix == nullptr) return false;
  while (*prefix != '\0') {
    if (asciiUpper(*value) != asciiUpper(*prefix)) return false;
    ++value;
    ++prefix;
  }
  return true;
}

bool containsTokenIgnoreCase(const char* value, const char* token) {
  if (value == nullptr || token == nullptr || token[0] == '\0') return false;
  for (const char* cursor = value; *cursor != '\0'; ++cursor) {
    const char* left = cursor;
    const char* right = token;
    while (*right != '\0' && asciiUpper(*left) == asciiUpper(*right)) {
      ++left;
      ++right;
    }
    if (*right == '\0') {
      const bool start_ok = cursor == value || *(cursor - 1) == ' ' ||
                            *(cursor - 1) == '-' || *(cursor - 1) == '_';
      const bool end_ok = *left == '\0' || *left == ' ' || *left == '-' ||
                          *left == '_';
      if (start_ok && end_ok) return true;
    }
  }
  return false;
}

}  // namespace

bool parseCscMeasurement(const uint8_t* data, size_t length,
                         CscMeasurement& measurement) {
  if (data == nullptr || length < 1) return false;
  CscMeasurement decoded = {};
  const uint8_t flags = data[0];
  decoded.wheel_present = (flags & kCscMeasurementFlagWheelPresent) != 0;
  decoded.crank_present = (flags & kCscMeasurementFlagCrankPresent) != 0;
  size_t offset = 1;
  if (decoded.wheel_present) {
    if (length < offset + 6u) return false;
    decoded.cumulative_wheel_revolutions = readU32(data + offset);
    decoded.last_wheel_event_time = readU16(data + offset + 4);
    offset += 6;
  }
  if (decoded.crank_present) {
    if (length < offset + 4u) return false;
    decoded.cumulative_crank_revolutions = readU16(data + offset);
    decoded.last_crank_event_time = readU16(data + offset + 2);
  }
  measurement = decoded;
  return true;
}

bool cscBondIsValid(const CscBondData& bond) {
  if ((bond.flags & kCscBondFlagValid) == 0) return false;
  for (uint8_t byte : bond.address) {
    if (byte != 0) return true;
  }
  return false;
}

void clearCscBond(CscBondData& bond) { bond = CscBondData{}; }

bool cscNameLooksLikeCycplus(const char* name) {
  if (name == nullptr || name[0] == '\0') return false;
  if (startsWithIgnoreCase(name, "CYCPLUS")) return true;
  return cscNameLooksLikeCadenceSensor(name) ||
         cscNameLooksLikeSpeedSensor(name);
}

bool cscNameLooksLikeSpeedSensor(const char* name) {
  return containsTokenIgnoreCase(name, "S3");
}

bool cscNameLooksLikeCadenceSensor(const char* name) {
  return containsTokenIgnoreCase(name, "C3");
}

uint32_t cscMeanIntervalUs(uint16_t event_time_delta, uint32_t revolutions) {
  if (event_time_delta == 0 || revolutions == 0) return 0;
  return static_cast<uint32_t>(
      (static_cast<uint64_t>(event_time_delta) * 1000000ull) /
      (static_cast<uint64_t>(kCscEventTimeUnitHz) * revolutions));
}

void CscMotionTracker::reset() { *this = CscMotionTracker{}; }

void CscMotionTracker::ingest(const CscMeasurement& measurement,
                              uint32_t now_ms) {
  wheel_present_ = measurement.wheel_present;
  crank_present_ = measurement.crank_present;

  if (measurement.crank_present) {
    if (!has_crank_sample_) {
      prev_crank_revolutions_ = measurement.cumulative_crank_revolutions;
      prev_crank_event_time_ = measurement.last_crank_event_time;
      has_crank_sample_ = true;
      last_crank_event_ms_ = now_ms;
    } else {
      const uint16_t d_revs = static_cast<uint16_t>(
          measurement.cumulative_crank_revolutions - prev_crank_revolutions_);
      const uint16_t d_time = static_cast<uint16_t>(
          measurement.last_crank_event_time - prev_crank_event_time_);
      prev_crank_revolutions_ = measurement.cumulative_crank_revolutions;
      prev_crank_event_time_ = measurement.last_crank_event_time;
      if (d_revs != 0 && d_time != 0) {
        last_crank_event_ms_ = now_ms;
        const uint32_t rpm_x10 = static_cast<uint32_t>(
            (static_cast<uint64_t>(d_revs) * kCscEventTimeUnitHz * 600ull) /
            d_time);
        if (rpm_x10 > 0 && rpm_x10 <= kCscCadenceMaxX10) {
          cadence_x10_ = static_cast<uint16_t>(rpm_x10);
          cadence_valid_ = true;
        }
      }
    }
  }

  if (!measurement.wheel_present) return;

  if (!has_wheel_sample_) {
    prev_wheel_revolutions_ = measurement.cumulative_wheel_revolutions;
    prev_wheel_event_time_ = measurement.last_wheel_event_time;
    has_wheel_sample_ = true;
    had_wheel_event_ = true;
    last_wheel_event_ms_ = now_ms;
    return;
  }

  const uint32_t d_revs =
      measurement.cumulative_wheel_revolutions - prev_wheel_revolutions_;
  const uint16_t d_time = static_cast<uint16_t>(
      measurement.last_wheel_event_time - prev_wheel_event_time_);
  prev_wheel_revolutions_ = measurement.cumulative_wheel_revolutions;
  prev_wheel_event_time_ = measurement.last_wheel_event_time;
  last_wheel_event_ms_ = now_ms;
  had_wheel_event_ = true;
  if (d_revs == 0 || d_time == 0) return;
  uint32_t applied = d_revs;
  if (applied > kCscMaxWheelRevsPerNotify) applied = kCscMaxWheelRevsPerNotify;
  const uint32_t interval_us = cscMeanIntervalUs(d_time, d_revs);
  if (interval_us == 0) return;
  pending_wheel_revs_ = static_cast<uint8_t>(applied);
  pending_wheel_interval_us_ = interval_us;
}

void CscMotionTracker::poll(uint32_t now_ms) {
  if (cadence_valid_ &&
      static_cast<uint32_t>(now_ms - last_crank_event_ms_) >= kCscStaleMs) {
    cadence_x10_ = 0;
    cadence_valid_ = false;
  }
}

void CscMotionTracker::onDisconnect() {
  has_crank_sample_ = false;
  has_wheel_sample_ = false;
  cadence_valid_ = false;
  cadence_x10_ = 0;
  crank_present_ = false;
  pending_wheel_revs_ = 0;
  pending_wheel_interval_us_ = 0;
}

bool CscMotionTracker::speedSourceActive(uint32_t now_ms) const {
  if (!had_wheel_event_) return false;
  return static_cast<uint32_t>(now_ms - last_wheel_event_ms_) < kCscStaleMs;
}

uint32_t CscMotionTracker::lastCrankEventAgeMs(uint32_t now_ms) const {
  if (!has_crank_sample_) return 0xFFFFFFFFu;
  return static_cast<uint32_t>(now_ms - last_crank_event_ms_);
}

uint32_t CscMotionTracker::lastWheelEventAgeMs(uint32_t now_ms) const {
  if (!had_wheel_event_) return 0xFFFFFFFFu;
  return static_cast<uint32_t>(now_ms - last_wheel_event_ms_);
}

CscWheelDelta CscMotionTracker::takeWheelDelta() {
  CscWheelDelta delta;
  delta.revolutions = pending_wheel_revs_;
  delta.mean_interval_us = pending_wheel_interval_us_;
  pending_wheel_revs_ = 0;
  pending_wheel_interval_us_ = 0;
  return delta;
}

}  // namespace bike
