#pragma once

#include <stdint.h>

#include "config.h"

namespace bike {

// Runtime seed for Device Information flags / standard Battery Level.
// Full live uptime / bonding / pairing window remain É4.6+.
struct BleBootSeed {
  bool config_from_flash = false;
  bool display_ok = false;
  bool fs_ok = false;
  bool usb_connected = false;
  uint8_t battery_percent = 0;
};

// Adafruit Bluefruit peripheral: Bike Computer Configuration Service GATT table.
// É4.4 registers service + 7 characteristics (props, permissions, lengths, CCCD).
// Safe write stubs return ERR_NOT_SUPPORTED via Command Result until É4.8–4.11.
// Advertising name resolves BikeComp-XXXX → serial suffix; Tx Power in Scan Response.
class BleManager {
 public:
  bool begin(const DeviceConfig& config, const BleBootSeed& seed = {});
  bool isOk() const { return ok_; }

 private:
  bool ok_ = false;
};

}  // namespace bike
