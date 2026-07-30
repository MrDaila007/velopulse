#pragma once

#include <stdint.h>

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
// Safe write stubs return ERR_NOT_SUPPORTED via Command Result until É4.8–4.11.
// Advertising name resolves BikeComp-XXXX → serial suffix; Tx Power in Scan Response.
class BleManager {
 public:
  bool begin(const DeviceConfig& config, const BleBootSeed& seed = {});
  bool isOk() const { return ok_; }

  // Keep USB flag current for the next Device Info Read.
  void noteUsbPresent(bool usb_present);

 private:
  bool ok_ = false;
};

}  // namespace bike
