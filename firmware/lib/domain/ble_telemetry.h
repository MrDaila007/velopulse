#pragma once

#include <stdint.h>

#include "ble_protocol.h"
#include "types.h"

namespace bike {

// Adaptive Telemetry rates (protocol §6 / architecture §11.2).
constexpr uint32_t kTelemetryIntervalSubscribedMs = 1000;     // 1 Hz
constexpr uint32_t kTelemetryIntervalUnsubscribedMs = 5000;   // 0.2 Hz
constexpr uint32_t kTelemetryIntervalSensorTestMs = 200;      // 5 Hz (Э4.13)

enum class TelemetryPublishMode : uint8_t {
  kUnsubscribed = 0,  // refresh GATT value for Read only
  kSubscribed = 1,    // notify + write
  kSensorTest = 2,    // notify @ 5 Hz
};

struct TelemetryBuildInput {
  TripSnapshot trip;
  BatterySnapshot battery;
  bool display_on = true;
  bool ble_connected = false;
  bool low_power_idle = false;
  bool deep_sleep_pending = false;
  bool smoothing_enabled = true;
  bool units_imperial = false;
  bool had_pulse = false;
  bool sensor_stuck = false;
  uint32_t last_pulse_ms = 0;
  uint32_t now_ms = 0;
  uint16_t cadence_x10 = 0;
  uint8_t csc_flags = 0;
  uint32_t last_crank_event_age_ms = kTelemetryNoPulseAgeMs;
};

uint32_t telemetryPublishIntervalMs(TelemetryPublishMode mode);

// True when the first packet is due, or interval since last_publish_ms elapsed.
bool telemetryDue(uint32_t last_publish_ms,
                  uint32_t now_ms,
                  TelemetryPublishMode mode,
                  bool has_published);

TelemetryPublishMode selectTelemetryPublishMode(bool notify_enabled,
                                                bool sensor_test_active);

uint8_t buildTelemetryFlags(const TelemetryBuildInput& input);
SensorState mapTelemetrySensorState(const TelemetryBuildInput& input);
PowerState mapTelemetryPowerState(const TelemetryBuildInput& input);
uint32_t telemetryLastPulseAgeMs(const TelemetryBuildInput& input);

// Fills all fields except seq; caller assigns seq after increment.
void fillTelemetryPacket(TelemetryPacket& out, const TelemetryBuildInput& input);

// Monotonic wrap-around sequence used on every publish (notify or Read refresh).
uint16_t nextTelemetrySeq(uint16_t seq);

}  // namespace bike
