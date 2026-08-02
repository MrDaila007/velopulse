#include "ble_manager.h"

namespace bike {

bool BleManager::begin(const DeviceConfig& config, const BleBootSeed& seed) {
  (void)config;
  (void)seed;
  ok_ = false;
  return false;
}

void BleManager::noteUsbPresent(bool usb_present) { (void)usb_present; }

void BleManager::serviceTelemetry(const TelemetryBuildInput& input,
                                  uint32_t now_ms) {
  (void)input;
  (void)now_ms;
}

void BleManager::setSensorTestActive(bool active) {
  sensor_test_active_ = active;
}

bool BleManager::sensorTestActive() const { return sensor_test_active_; }

void BleManager::recordError(ErrorLogCode code, ErrorLogSeverity severity,
                             uint16_t detail, uint32_t now_ms) {
  (void)code;
  (void)severity;
  (void)detail;
  (void)now_ms;
}

void BleManager::openPairingWindow(uint16_t duration_s, uint32_t now_ms) {
  (void)duration_s;
  (void)now_ms;
}

void BleManager::noteMovement() {}

void BleManager::clearBonds() {}

bool BleManager::hasPendingConfigWrite() const { return false; }

bool BleManager::takePendingConfigWrite(uint8_t out[kConfigurationSize]) {
  (void)out;
  return false;
}

void BleManager::completePendingConfigWrite() {}

void BleManager::publishConfigWriteResult(const ConfigWriteResult& result) {
  (void)result;
}

void BleManager::publishAppliedConfig(const DeviceConfig& config,
                                      bool config_valid) {
  (void)config;
  (void)config_valid;
}

bool BleManager::hasPendingSafeCommand() const { return false; }

bool BleManager::takePendingSafeCommand(SafeCommandParseResult& command) {
  (void)command;
  return false;
}

void BleManager::completePendingSafeCommand() {}

void BleManager::publishSafeCommandResult(CommandId command_id,
                                          const BleCommandResult& result) {
  (void)command_id;
  (void)result;
}

bool BleManager::hasPendingDangerousCommand() const { return false; }

bool BleManager::takePendingDangerousCommand(
    DangerousCommandParseResult& command) {
  (void)command;
  return false;
}

void BleManager::completePendingDangerousCommand() {}

void BleManager::publishDangerousCommandResult(CommandId command_id,
                                               const BleCommandResult& result) {
  (void)command_id;
  (void)result;
}

}  // namespace bike
