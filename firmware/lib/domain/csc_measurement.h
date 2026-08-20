#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bike {

// Bluetooth SIG Cycling Speed and Cadence Measurement (UUID 0x2A5B).
constexpr uint8_t kCscMeasurementFlagWheelPresent = 1u << 0;
constexpr uint8_t kCscMeasurementFlagCrankPresent = 1u << 1;
constexpr uint16_t kCscEventTimeUnitHz = 1024;
constexpr uint16_t kCscCadenceMaxX10 = 3000;  // 300.0 rpm
constexpr uint32_t kCscStaleMs = 4000;
constexpr uint8_t kCscBondFlagValid = 1u << 0;
constexpr size_t kCscBondNameSize = 16;
constexpr size_t kCscAddressSize = 6;
constexpr uint8_t kCscMaxWheelRevsPerNotify = 16;

struct CscMeasurement {
  bool wheel_present = false;
  bool crank_present = false;
  uint32_t cumulative_wheel_revolutions = 0;
  uint16_t last_wheel_event_time = 0;  // 1/1024 s
  uint16_t cumulative_crank_revolutions = 0;
  uint16_t last_crank_event_time = 0;  // 1/1024 s
};

struct CscBondData {
  uint8_t address[kCscAddressSize] = {};
  uint8_t address_type = 0;
  uint8_t flags = 0;
  char name[kCscBondNameSize] = {};
};

struct CscWheelDelta {
  uint8_t revolutions = 0;
  uint32_t mean_interval_us = 0;
};

struct CscSnapshot {
  bool connected = false;
  bool pairing = false;
  bool bonded = false;
  bool wheel_present = false;
  bool crank_present = false;
  bool cadence_valid = false;
  bool speed_source_active = false;
  uint16_t cadence_x10 = 0;  // 0.1 rpm
  uint32_t last_crank_event_age_ms = 0xFFFFFFFFu;
  uint32_t last_wheel_event_age_ms = 0xFFFFFFFFu;
  char name[kCscBondNameSize] = {};
};

bool parseCscMeasurement(const uint8_t* data, size_t length,
                         CscMeasurement& measurement);

bool cscBondIsValid(const CscBondData& bond);
void clearCscBond(CscBondData& bond);

// True for CYCPLUS C3/S3 advertising names (and close variants).
bool cscNameLooksLikeCycplus(const char* name);
bool cscNameLooksLikeSpeedSensor(const char* name);
bool cscNameLooksLikeCadenceSensor(const char* name);

// Converts CSC wheel/crank event-time deltas (1/1024 s) into a mean pulse
// interval. Returns 0 when the delta is empty or would overflow.
uint32_t cscMeanIntervalUs(uint16_t event_time_delta, uint32_t revolutions);

class CscMotionTracker {
 public:
  void reset();
  void ingest(const CscMeasurement& measurement, uint32_t now_ms);
  void poll(uint32_t now_ms);
  // Re-seed crank/wheel deltas on the next notify without dropping the 4 s
  // Hall-suppress window used while S3 is the live speed source.
  void onDisconnect();

  uint16_t cadenceX10() const { return cadence_x10_; }
  bool cadenceValid() const { return cadence_valid_; }
  bool wheelPresent() const { return wheel_present_; }
  bool crankPresent() const { return crank_present_; }
  bool speedSourceActive(uint32_t now_ms) const;
  uint32_t lastCrankEventAgeMs(uint32_t now_ms) const;
  uint32_t lastWheelEventAgeMs(uint32_t now_ms) const;
  CscWheelDelta takeWheelDelta();

 private:
  bool has_crank_sample_ = false;
  bool has_wheel_sample_ = false;
  bool had_wheel_event_ = false;
  bool wheel_present_ = false;
  bool crank_present_ = false;
  bool cadence_valid_ = false;
  uint16_t prev_crank_revolutions_ = 0;
  uint16_t prev_crank_event_time_ = 0;
  uint32_t prev_wheel_revolutions_ = 0;
  uint16_t prev_wheel_event_time_ = 0;
  uint16_t cadence_x10_ = 0;
  uint32_t last_crank_event_ms_ = 0;
  uint32_t last_wheel_event_ms_ = 0;
  uint8_t pending_wheel_revs_ = 0;
  uint32_t pending_wheel_interval_us_ = 0;
};

}  // namespace bike
