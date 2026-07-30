#include "ble_manager.h"

#include <Arduino.h>
#include <bluefruit.h>
#include <string.h>

#include "ble_identity.h"
#include "ble_protocol.h"
#include "protocol_codec.h"

namespace bike {
namespace {

BLEService g_service(kBleServiceUuid);
BLECharacteristic g_device_info(kBleDeviceInfoUuid);
BLECharacteristic g_telemetry(kBleTelemetryUuid);
BLECharacteristic g_config_read(kBleConfigReadUuid);
BLECharacteristic g_config_write(kBleConfigWriteUuid);
BLECharacteristic g_command(kBleCommandUuid);
BLECharacteristic g_command_result(kBleCommandResultUuid);
BLECharacteristic g_error_log(kBleErrorLogUuid);

uint8_t g_device_info_buf[kDeviceInfoSize] = {};
uint8_t g_telemetry_buf[kTelemetrySize] = {};
uint8_t g_config_read_buf[kConfigurationSize] = {};
uint8_t g_command_result_buf[kCommandResultMaxSize] = {};
uint8_t g_error_log_buf[kErrorLogMaxSize] = {};

char g_local_name[16] = {};

bool beginChar(BLECharacteristic& chr) {
  return chr.begin() == ERROR_NONE;
}

void readFicrSerial(uint8_t serial[8], uint16_t& suffix_hex) {
  const uint32_t id0 = NRF_FICR->DEVICEID[0];
  const uint32_t id1 = NRF_FICR->DEVICEID[1];
  serial[0] = static_cast<uint8_t>(id0);
  serial[1] = static_cast<uint8_t>(id0 >> 8);
  serial[2] = static_cast<uint8_t>(id0 >> 16);
  serial[3] = static_cast<uint8_t>(id0 >> 24);
  serial[4] = static_cast<uint8_t>(id1);
  serial[5] = static_cast<uint8_t>(id1 >> 8);
  serial[6] = static_cast<uint8_t>(id1 >> 16);
  serial[7] = static_cast<uint8_t>(id1 >> 24);
  suffix_hex = static_cast<uint16_t>(id0 & 0xFFFFu);
}

void publishCommandResult(uint8_t command_id, CommandStatus status) {
  CommandResultPacket result = {};
  result.struct_version = kBleStructVersion;
  result.command_id = command_id;
  result.status = static_cast<uint8_t>(status);
  const size_t encoded =
      encodeCommandResult(result, g_command_result_buf, sizeof(g_command_result_buf));
  if (encoded == 0) return;
  g_command_result.write(g_command_result_buf, static_cast<uint16_t>(encoded));
  g_command_result.notify(g_command_result_buf, static_cast<uint16_t>(encoded));
}

void onConfigWrite(uint16_t, BLECharacteristic*, uint8_t*, uint16_t) {
  publishCommandResult(kCommandResultConfigWriteId, CommandStatus::kErrNotSupported);
}

void onCommandWrite(uint16_t, BLECharacteristic*, uint8_t* data, uint16_t len) {
  uint8_t command_id = 0;
  if (data != nullptr && len >= 2) command_id = data[1];
  publishCommandResult(command_id, CommandStatus::kErrNotSupported);
}

bool registerGatt(const DeviceConfig& config, const BleBootSeed& seed,
                  const uint8_t serial[8]) {
  if (g_service.begin() != ERROR_NONE) {
    return false;
  }

  g_device_info.setProperties(CHR_PROPS_READ);
  g_device_info.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  g_device_info.setFixedLen(kDeviceInfoSize);
  g_device_info.setBuffer(g_device_info_buf, sizeof(g_device_info_buf));
  g_device_info.setUserDescriptor("Device Information");
  if (!beginChar(g_device_info)) return false;

  g_telemetry.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  g_telemetry.setPermission(SECMODE_ENC_NO_MITM, SECMODE_NO_ACCESS);
  g_telemetry.setFixedLen(kTelemetrySize);
  g_telemetry.setBuffer(g_telemetry_buf, sizeof(g_telemetry_buf));
  g_telemetry.setUserDescriptor("Telemetry");
  if (!beginChar(g_telemetry)) return false;

  g_config_read.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  g_config_read.setPermission(SECMODE_ENC_NO_MITM, SECMODE_NO_ACCESS);
  g_config_read.setFixedLen(kConfigurationSize);
  g_config_read.setBuffer(g_config_read_buf, sizeof(g_config_read_buf));
  g_config_read.setUserDescriptor("Config Read");
  if (!beginChar(g_config_read)) return false;

  g_config_write.setProperties(CHR_PROPS_WRITE);
  g_config_write.setPermission(SECMODE_NO_ACCESS, SECMODE_ENC_NO_MITM);
  g_config_write.setFixedLen(kConfigurationSize);
  g_config_write.setUserDescriptor("Config Write");
  g_config_write.setWriteCallback(onConfigWrite);
  if (!beginChar(g_config_write)) return false;

  g_command.setProperties(CHR_PROPS_WRITE);
  g_command.setPermission(SECMODE_NO_ACCESS, SECMODE_ENC_NO_MITM);
  g_command.setMaxLen(kCommandMaxSize);
  g_command.setUserDescriptor("Command");
  g_command.setWriteCallback(onCommandWrite);
  if (!beginChar(g_command)) return false;

  g_command_result.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  g_command_result.setPermission(SECMODE_ENC_NO_MITM, SECMODE_NO_ACCESS);
  g_command_result.setMaxLen(kCommandResultMaxSize);
  g_command_result.setBuffer(g_command_result_buf, sizeof(g_command_result_buf));
  g_command_result.setUserDescriptor("Command Result");
  if (!beginChar(g_command_result)) return false;

  g_error_log.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  g_error_log.setPermission(SECMODE_ENC_NO_MITM, SECMODE_NO_ACCESS);
  g_error_log.setMaxLen(kErrorLogMaxSize);
  g_error_log.setBuffer(g_error_log_buf, sizeof(g_error_log_buf));
  g_error_log.setUserDescriptor("Error Log");
  if (!beginChar(g_error_log)) return false;

  DeviceInfoPacket info = {};
  info.struct_version = kBleStructVersion;
  info.proto_major = kBleProtoMajor;
  info.proto_minor = kBleProtoMinor;
  info.hw_revision = static_cast<uint8_t>(HW_REVISION);
  strncpy(info.model, "BIKECOMP-XIAO", sizeof(info.model) - 1u);
  strncpy(info.fw_version, FW_VERSION, sizeof(info.fw_version) - 1u);
  memcpy(info.serial, serial, 8);
  if (seed.config_from_flash) info.flags |= kDeviceInfoFlagConfigValid;
  if (seed.display_ok) info.flags |= kDeviceInfoFlagDisplayOk;
  if (seed.fs_ok) info.flags |= kDeviceInfoFlagFsOk;
  if (seed.usb_connected) info.flags |= kDeviceInfoFlagUsbConnected;
  encodeDeviceInfo(info, g_device_info_buf);
  g_device_info.write(g_device_info_buf, kDeviceInfoSize);

  encodeConfiguration(config, g_config_read_buf);
  g_config_read.write(g_config_read_buf, kConfigurationSize);

  TelemetryPacket telemetry = {};
  telemetry.struct_version = kBleStructVersion;
  telemetry.last_pulse_age_ms = kTelemetryNoPulseAgeMs;
  telemetry.sensor_state = static_cast<uint8_t>(SensorState::kNoSignal);
  telemetry.battery_pct = seed.battery_percent;
  encodeTelemetry(telemetry, g_telemetry_buf);
  g_telemetry.write(g_telemetry_buf, kTelemetrySize);

  CommandResultPacket idle = {};
  idle.struct_version = kBleStructVersion;
  idle.status = static_cast<uint8_t>(CommandStatus::kOk);
  const size_t result_len =
      encodeCommandResult(idle, g_command_result_buf, sizeof(g_command_result_buf));
  g_command_result.write(g_command_result_buf, static_cast<uint16_t>(result_len));

  ErrorLogPacket log = {};
  log.struct_version = kBleStructVersion;
  log.entry_count = 1;
  log.entries[0].severity = static_cast<uint8_t>(ErrorLogSeverity::kInfo);
  const size_t log_len =
      encodeErrorLog(log, g_error_log_buf, sizeof(g_error_log_buf));
  if (log_len > 0) {
    g_error_log.write(g_error_log_buf, static_cast<uint16_t>(log_len));
  }
  return true;
}

void startAdvertising() {
  Bluefruit.setName(g_local_name);
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(g_service);
  Bluefruit.ScanResponse.addName();
  Bluefruit.ScanResponse.addTxPower();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setIntervalMS(30, 1000);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

}  // namespace

bool BleManager::begin(const DeviceConfig& config, const BleBootSeed& seed) {
  ok_ = false;

  uint8_t serial[8] = {};
  uint16_t serial_suffix = 0;
  readFicrSerial(serial, serial_suffix);
  resolveDeviceLocalName(config.device_name, serial_suffix, g_local_name,
                         sizeof(g_local_name));

  Bluefruit.configUuid128Count(8);
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  if (!Bluefruit.begin()) {
    return false;
  }

  Bluefruit.setTxPower(4);
  Bluefruit.Periph.setConnInterval(12, 48);
  Bluefruit.Security.setIOCaps(false, false, false);
  Bluefruit.Security.setMITM(false);

  if (!registerGatt(config, seed, serial)) {
    Bluefruit.Advertising.stop();
    return false;
  }

  startAdvertising();
  Serial.print("BLE ADV name: ");
  Serial.println(g_local_name);
  ok_ = true;
  return true;
}

}  // namespace bike
