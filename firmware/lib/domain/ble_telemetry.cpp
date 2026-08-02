#include "ble_telemetry.h"

namespace bike {
namespace {

uint32_t saturatingDivU64(uint64_t value, uint64_t divisor) {
  if (divisor == 0u) return 0;
  const uint64_t quot = value / divisor;
  if (quot > 0xFFFFFFFFull) return 0xFFFFFFFFu;
  return static_cast<uint32_t>(quot);
}

}  // namespace

uint32_t telemetryPublishIntervalMs(TelemetryPublishMode mode) {
  switch (mode) {
    case TelemetryPublishMode::kSensorTest:
      return kTelemetryIntervalSensorTestMs;
    case TelemetryPublishMode::kSubscribed:
      return kTelemetryIntervalSubscribedMs;
    case TelemetryPublishMode::kUnsubscribed:
    default:
      return kTelemetryIntervalUnsubscribedMs;
  }
}

bool telemetryDue(uint32_t last_publish_ms,
                  uint32_t now_ms,
                  TelemetryPublishMode mode,
                  bool has_published) {
  if (!has_published) return true;
  const uint32_t elapsed = static_cast<uint32_t>(now_ms - last_publish_ms);
  return elapsed >= telemetryPublishIntervalMs(mode);
}

TelemetryPublishMode selectTelemetryPublishMode(bool notify_enabled,
                                                bool sensor_test_active) {
  if (sensor_test_active) return TelemetryPublishMode::kSensorTest;
  if (notify_enabled) return TelemetryPublishMode::kSubscribed;
  return TelemetryPublishMode::kUnsubscribed;
}

uint8_t buildTelemetryFlags(const TelemetryBuildInput& input) {
  uint8_t flags = 0;
  if (input.trip.ride_state == RideState::kMoving) {
    flags |= kTelemetryFlagMoving;
  }
  if (input.display_on) flags |= kTelemetryFlagDisplayOn;
  if (input.battery.charge_status == ChargeStatus::kCharging) {
    flags |= kTelemetryFlagCharging;
  }
  if (input.battery.usb_present) flags |= kTelemetryFlagUsbConnected;
  if (input.battery.low_battery) flags |= kTelemetryFlagLowBattery;
  if (input.smoothing_enabled) flags |= kTelemetryFlagSmoothingEnabled;
  if (input.units_imperial) flags |= kTelemetryFlagUnitsImperial;
  if (input.battery.charge_status == ChargeStatus::kUnknown) {
    flags |= kTelemetryFlagChargeStatusUnknown;
  }
  return flags;
}

SensorState mapTelemetrySensorState(const TelemetryBuildInput& input) {
  if (input.sensor_stuck) return SensorState::kStuck;
  if (!input.had_pulse) return SensorState::kNoSignal;
  if (input.trip.ride_state == RideState::kMoving) return SensorState::kOk;
  return SensorState::kIdle;
}

PowerState mapTelemetryPowerState(const TelemetryBuildInput& input) {
  if (input.battery.charge_status == ChargeStatus::kCharging) {
    return PowerState::kCharging;
  }
  if (input.ble_connected) return PowerState::kBleConfig;
  if (input.deep_sleep_pending) return PowerState::kDeepSleepPending;
  if (!input.display_on || input.low_power_idle) {
    return PowerState::kIdleDisplayOff;
  }
  if (input.trip.ride_state == RideState::kPaused) {
    return PowerState::kShortStop;
  }
  return PowerState::kActive;
}

uint32_t telemetryLastPulseAgeMs(const TelemetryBuildInput& input) {
  if (!input.had_pulse) return kTelemetryNoPulseAgeMs;
  return static_cast<uint32_t>(input.now_ms - input.last_pulse_ms);
}

void fillTelemetryPacket(TelemetryPacket& out, const TelemetryBuildInput& input) {
  out = {};
  out.struct_version = kBleStructVersion;
  out.flags = buildTelemetryFlags(input);
  out.speed_x100 = input.trip.speed_x100;
  out.avg_speed_x100 = input.trip.average_speed_x100;
  out.max_speed_x100 = input.trip.max_speed_x100;
  out.trip_distance_cm = input.trip.trip_distance_mm / 10u;
  out.moving_time_s = input.trip.moving_time_ms / 1000u;
  out.odometer_m = saturatingDivU64(input.trip.odometer_mm, 1000ull);
  out.battery_mv = input.battery.millivolts;
  out.battery_pct = input.battery.percent > 100u ? 100u : input.battery.percent;
  out.ride_state = static_cast<uint8_t>(input.trip.ride_state);
  out.revolutions = input.trip.revolutions;
  out.last_pulse_age_ms = telemetryLastPulseAgeMs(input);
  out.sensor_state = static_cast<uint8_t>(mapTelemetrySensorState(input));
  out.power_state = static_cast<uint8_t>(mapTelemetryPowerState(input));
}

uint16_t nextTelemetrySeq(uint16_t seq) {
  return static_cast<uint16_t>(seq + 1u);
}

}  // namespace bike
