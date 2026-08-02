#include "protocol_samples.h"

#include <cstring>

namespace bike {
namespace test_support {

DeviceInfoPacket makeNominalDeviceInfo() {
  DeviceInfoPacket info = {};
  info.struct_version = kBleStructVersion;
  info.proto_major = kBleProtoMajor;
  info.proto_minor = kBleProtoMinor;
  info.hw_revision = 1;
  memcpy(info.model, "BIKECOMP-XIAO", 13);
  memcpy(info.fw_version, "1.0.0", 5);
  const uint8_t serial[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  memcpy(info.serial, serial, 8);
  info.uptime_s = 3600;
  info.reset_reason = static_cast<uint8_t>(ResetReason::kPowerOn);
  info.boot_count = 42;
  info.flags = kDeviceInfoFlagConfigValid | kDeviceInfoFlagDisplayOk |
               kDeviceInfoFlagFsOk;
  return info;
}

TelemetryPacket makeMovingTelemetry() {
  TelemetryPacket telemetry = {};
  telemetry.struct_version = kBleStructVersion;
  telemetry.flags = kTelemetryFlagMoving | kTelemetryFlagDisplayOn |
                    kTelemetryFlagSmoothingEnabled;
  telemetry.speed_x100 = 2550;
  telemetry.avg_speed_x100 = 2200;
  telemetry.max_speed_x100 = 3500;
  telemetry.trip_distance_cm = 125000;
  telemetry.moving_time_s = 1800;
  telemetry.odometer_m = 123456;
  telemetry.battery_mv = 3900;
  telemetry.battery_pct = 75;
  telemetry.ride_state = static_cast<uint8_t>(RideState::kMoving);
  telemetry.revolutions = 500;
  telemetry.last_pulse_age_ms = 250;
  telemetry.seq = 42;
  telemetry.sensor_state = static_cast<uint8_t>(SensorState::kOk);
  telemetry.power_state = static_cast<uint8_t>(PowerState::kActive);
  return telemetry;
}

TelemetryPacket makePausedTelemetry() {
  TelemetryPacket telemetry = makeMovingTelemetry();
  telemetry.flags = kTelemetryFlagDisplayOn | kTelemetryFlagSmoothingEnabled;
  telemetry.speed_x100 = 0;
  telemetry.ride_state = static_cast<uint8_t>(RideState::kPaused);
  telemetry.last_pulse_age_ms = 5000;
  telemetry.seq = 43;
  telemetry.sensor_state = static_cast<uint8_t>(SensorState::kIdle);
  telemetry.power_state = static_cast<uint8_t>(PowerState::kShortStop);
  return telemetry;
}

}  // namespace test_support
}  // namespace bike
