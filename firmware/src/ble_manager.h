#pragma once

#include <stdint.h>

#include "ble_telemetry.h"
#include "config.h"

namespace bike {

// Runtime seed for Device Information and initial Telemetry battery %.
struct BleBootSeed {
  bool config_from_flash = false;
  bool display_ok = false;
  bool fs_ok = false;
  bool usb_connected = false;
  uint8_t battery_percent = 0;
  uint8_t reset_reason = 0;
  uint16_t boot_count = 0;
  uint32_t boot_ms = 0;
  // Prototype default: keep pairing window open (FEATURE_OPEN_PAIRING).
  bool open_pairing_always = true;
};

// Adafruit Bluefruit peripheral: Bike Computer Configuration Service GATT table.
// É4.4 registers service + 7 characteristics (props, permissions, lengths, CCCD).
// É4.6 refreshes Device Info on each Read (uptime / live flags / bonded).
// É4.7 publishes Telemetry at adaptive rate (1 Hz notify / 0.2 Hz Read refresh).
// Safe write stubs return ERR_NOT_SUPPORTED via Command Result until É4.8–4.11.
// Advertising name resolves BikeComp-XXXX → serial suffix; Tx Power in Scan Response.
class BleManager {
 public:
  bool begin(const DeviceConfig& config, const BleBootSeed& seed = {});
  bool isOk() const { return ok_; }

  // Keep USB flag current for the next Device Info Read.
  void noteUsbPresent(bool usb_present);

  // Build + publish Telemetry when the adaptive interval elapses.
  // Notifies when CCCD is enabled (or sensor-test mode); otherwise Write-only.
  void serviceTelemetry(const TelemetryBuildInput& input, uint32_t now_ms);

  // Sensor-test mode forces 5 Hz notify (Э4.13 command path will toggle this).
  void setSensorTestActive(bool active);
  bool sensorTestActive() const;

 private:
  bool ok_ = false;
};

}  // namespace bike
