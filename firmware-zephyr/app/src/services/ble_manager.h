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

struct BleBootSeed {
  bool config_from_flash = false;
  bool display_ok = false;
  bool fs_ok = false;
  bool usb_connected = false;
  uint8_t battery_percent = 0;
  uint8_t reset_reason = 0;
  uint16_t boot_count = 0;
  uint32_t boot_ms = 0;
  bool open_pairing_always = false;
};

class BleManager {
 public:
  bool begin(const DeviceConfig& config, const BleBootSeed& seed = {});
  bool isOk() const { return ok_; }
  bool bleAdvertising() const { return ble_advertising_; }
  bool bleConnected() const { return ble_connected_; }

  void noteUsbPresent(bool usb_present);
  void serviceTelemetry(const TelemetryBuildInput& input, uint32_t now_ms);
  void setSensorTestActive(bool active);
  bool sensorTestActive() const;
  void recordError(ErrorLogCode code,
                   ErrorLogSeverity severity,
                   uint16_t detail,
                   uint32_t now_ms);
  void openPairingWindow(uint16_t duration_s, uint32_t now_ms);
  void noteMovement();
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
  bool sensor_test_active_ = false;
  bool ble_advertising_ = false;
  bool ble_connected_ = false;
};

}  // namespace bike
