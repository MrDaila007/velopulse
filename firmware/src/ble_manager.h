#pragma once

#include <stdint.h>

#include "ble_command.h"
#include "ble_telemetry.h"
#include "config.h"

namespace bike {

struct ConfigWriteResult {
  CommandStatus status = CommandStatus::kOk;
  uint8_t field_id = 0;
};

struct BleCommandResult {
  CommandStatus status = CommandStatus::kOk;
  uint8_t detail = 0;
  uint32_t token = 0;
  uint8_t payload[kCommandResultMaxPayload] = {};
  uint8_t payload_len = 0;
};

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
// É4.8 stages Config Write in a pending queue; main loop validates/applies.
// Safe commands are staged from the BLE callback and executed by AppController.
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
  void openPairingWindow(uint16_t duration_s, uint32_t now_ms);

  void clearBonds();

  bool hasPendingConfigWrite() const;
  bool takePendingConfigWrite(uint8_t out[kConfigurationSize]);
  void completePendingConfigWrite();

  void publishConfigWriteResult(const ConfigWriteResult& result);
  void publishAppliedConfig(const DeviceConfig& config, bool config_valid);

  bool hasPendingSafeCommand() const;
  bool takePendingSafeCommand(SafeCommandParseResult& command);
  void completePendingSafeCommand();
  void publishSafeCommandResult(CommandId command_id,
                                const BleCommandResult& result);
  bool hasPendingDangerousCommand() const;
  bool takePendingDangerousCommand(DangerousCommandParseResult& command);
  void completePendingDangerousCommand();
  void publishDangerousCommandResult(CommandId command_id,
                                     const BleCommandResult& result);

 private:
  bool ok_ = false;
};

}  // namespace bike
