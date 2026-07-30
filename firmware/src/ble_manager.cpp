#include "ble_manager.h"

#include <Arduino.h>
#include <bluefruit.h>
#include <nrf_soc.h>
#include <string.h>

#include "ble_command.h"
#include "ble_device_info.h"
#include "ble_identity.h"
#include "ble_protocol.h"
#include "ble_config_write.h"
#include "ble_telemetry.h"
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

DeviceInfoPacket g_device_info_packet = {};
uint32_t g_boot_ms = 0;
uint32_t g_pairing_window_started_ms = 0;
uint32_t g_pairing_window_ms = kDefaultPairingWindowMs;
bool g_open_pairing_always = false;
bool g_config_valid = false;
bool g_display_ok = false;
bool g_fs_ok = false;
bool g_usb_connected = false;
bool g_deep_sleep_supported = false;

uint16_t g_telemetry_seq = 0;
uint32_t g_telemetry_last_publish_ms = 0;
bool g_telemetry_has_published = false;
bool g_sensor_test_active = false;
uint16_t g_serial_suffix = 0;
uint16_t g_pairing_allowed_conn_hdl = BLE_CONN_HANDLE_INVALID;
PendingConfigWrite g_pending_config_write = {};
PendingSafeCommand g_pending_safe_command = {};
PendingDangerousCommand g_pending_dangerous_command = {};
DangerousCommandSession g_dangerous_command_session = {};

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

bool connectionBonded(uint16_t conn_hdl) {
  BLEConnection* conn = Bluefruit.Connection(conn_hdl);
  return conn != nullptr && conn->bonded();
}

bool connectionTrusted(uint16_t conn_hdl) {
  BLEConnection* conn = Bluefruit.Connection(conn_hdl);
  return conn != nullptr && conn->bonded() && conn->secured();
}

uint32_t generateHardwareNonce() {
  uint8_t available = 0;
  if (sd_rand_application_bytes_available_get(&available) != NRF_SUCCESS ||
      available < sizeof(uint32_t)) {
    return 0;
  }
  uint32_t token = 0;
  if (sd_rand_application_vector_get(reinterpret_cast<uint8_t*>(&token),
                                     sizeof(token)) != NRF_SUCCESS) {
    return 0;
  }
  return token;
}

void encodeAndPublishDeviceInfo() {
  encodeDeviceInfo(g_device_info_packet, g_device_info_buf);
  g_device_info.write(g_device_info_buf, kDeviceInfoSize);
}

void refreshDeviceInfoForRead(uint16_t conn_hdl) {
  refreshDeviceInfoLiveFields(
      g_device_info_packet, g_boot_ms, millis(), g_pairing_window_started_ms,
      g_pairing_window_ms,
      g_open_pairing_always, connectionBonded(conn_hdl), g_usb_connected,
      g_config_valid, g_display_ok, g_fs_ok, g_deep_sleep_supported);
  encodeDeviceInfo(g_device_info_packet, g_device_info_buf);
}

// Must reply synchronously (useAdaCallback=false); SoftDevice waits for
// sd_ble_gatts_rw_authorize_reply before completing the ATT Read.
void onDeviceInfoRead(uint16_t conn_hdl, BLECharacteristic*,
                      ble_gatts_evt_read_t* request) {
  ble_gatts_rw_authorize_reply_params_t reply = {};
  reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
  reply.params.read.gatt_status = BLE_GATT_STATUS_SUCCESS;
  reply.params.read.update = 0;
  if (request != nullptr && request->offset == 0) {
    refreshDeviceInfoForRead(conn_hdl);
    reply.params.read.update = 1;
    reply.params.read.offset = 0;
    reply.params.read.len = kDeviceInfoSize;
    reply.params.read.p_data = g_device_info_buf;
  }
  sd_ble_gatts_rw_authorize_reply(conn_hdl, &reply);
}

void publishCommandResult(uint8_t command_id,
                            CommandStatus status,
                            uint8_t detail = 0,
                            uint32_t token = 0,
                            const uint8_t* payload = nullptr,
                            uint8_t payload_len = 0) {
  CommandResultPacket result = {};
  result.struct_version = kBleStructVersion;
  result.command_id = command_id;
  result.status = static_cast<uint8_t>(status);
  result.detail = detail;
  result.token = token;
  if (payload != nullptr && payload_len <= kCommandResultMaxPayload) {
    result.payload_len = payload_len;
    memcpy(result.payload, payload, payload_len);
  }
  const size_t encoded =
      encodeCommandResult(result, g_command_result_buf, sizeof(g_command_result_buf));
  if (encoded == 0) return;
  g_command_result.write(g_command_result_buf, static_cast<uint16_t>(encoded));
  g_command_result.notify(g_command_result_buf, static_cast<uint16_t>(encoded));
}

void publishConfigRead(const DeviceConfig& config) {
  encodeConfiguration(config, g_config_read_buf);
  g_config_read.write(g_config_read_buf, kConfigurationSize);
  g_config_read.notify(g_config_read_buf, kConfigurationSize);
}

void refreshAdvertisingName(const DeviceConfig& config) {
  resolveDeviceLocalName(config.device_name, g_serial_suffix, g_local_name,
                         sizeof(g_local_name));
  Bluefruit.setName(g_local_name);
}

void onConfigWrite(uint16_t, BLECharacteristic*, uint8_t* data, uint16_t len) {
  ConfigWriteRejectReason reject = ConfigWriteRejectReason::kNone;
  if (!configWriteQueueStage(g_pending_config_write, data, len, reject)) {
    if (reject == ConfigWriteRejectReason::kBusy) {
      publishCommandResult(kCommandResultConfigWriteId, CommandStatus::kErrBusy);
      return;
    }
    publishCommandResult(kCommandResultConfigWriteId, CommandStatus::kErrRange,
                         static_cast<uint8_t>(ConfigFieldId::kStructVersion));
    return;
  }
}

void onCommandWrite(uint16_t conn_hdl, BLECharacteristic*, uint8_t* data,
                    uint16_t len) {
  uint8_t command_id = 0;
  if (data != nullptr && len >= 2) command_id = data[1];

  if (isDangerousCommandId(command_id)) {
    const DangerousCommandParseResult parsed =
        parseDangerousBleCommand(data, len);
    if (!parsed.ok) {
      publishCommandResult(command_id, parsed.status, parsed.field_id);
      return;
    }
    if (g_pending_safe_command.state == SafeCommandQueueState::kPending ||
        g_pending_dangerous_command.state ==
            DangerousCommandQueueState::kPending) {
      publishCommandResult(command_id, CommandStatus::kErrBusy);
      return;
    }
    const bool bonded = connectionTrusted(conn_hdl);
    const uint32_t generated_token =
        !parsed.has_token && bonded ? generateHardwareNonce() : 0;
    const DangerousCommandHandshakeResult handshake =
        processDangerousCommandHandshake(g_dangerous_command_session, parsed,
                                         conn_hdl, bonded, millis(),
                                         generated_token);
    if (!handshake.execute) {
      publishCommandResult(command_id, handshake.status, handshake.field_id,
                           handshake.token);
      return;
    }
    if (!dangerousCommandQueueStage(g_pending_dangerous_command, parsed)) {
      publishCommandResult(command_id, CommandStatus::kErrBusy);
    }
    return;
  }

  const SafeCommandParseResult parsed = parseSafeBleCommand(data, len);
  if (!parsed.ok) {
    publishCommandResult(command_id, parsed.status, parsed.field_id);
    return;
  }
  if (g_pending_dangerous_command.state ==
          DangerousCommandQueueState::kPending ||
      !safeCommandQueueStage(g_pending_safe_command, parsed)) {
    publishCommandResult(command_id, CommandStatus::kErrBusy);
  }
}

void onBleEvent(ble_evt_t* event) {
  if (event == nullptr ||
      event->header.evt_id != BLE_GAP_EVT_SEC_PARAMS_REQUEST) {
    return;
  }

  const uint16_t conn_hdl = event->evt.common_evt.conn_handle;
  if (shouldRejectPairingRequest(g_pairing_window_started_ms, millis(),
                                 g_pairing_window_ms, g_open_pairing_always,
                                 connectionBonded(conn_hdl))) {
    // Bluefruit invokes its security handler before this public event hook.
    // Disconnect immediately to abort the accepted-on-wire pairing exchange.
    g_pairing_allowed_conn_hdl = BLE_CONN_HANDLE_INVALID;
    Bluefruit.disconnect(conn_hdl);
    return;
  }
  g_pairing_allowed_conn_hdl = conn_hdl;
}

void onPairComplete(uint16_t conn_hdl, uint8_t auth_status) {
  const bool was_allowed = g_pairing_allowed_conn_hdl == conn_hdl;
  g_pairing_allowed_conn_hdl = BLE_CONN_HANDLE_INVALID;
  if (auth_status != BLE_GAP_SEC_STATUS_SUCCESS || was_allowed) return;

  // Defense in depth: never persist a pairing that completed without passing
  // the window gate (for example if disconnect delivery was delayed).
  BLEConnection* conn = Bluefruit.Connection(conn_hdl);
  if (conn != nullptr && conn->bonded()) conn->removeBondKey();
  Bluefruit.disconnect(conn_hdl);
}

void onDisconnect(uint16_t conn_hdl, uint8_t) {
  g_sensor_test_active = false;
  safeCommandQueueFinish(g_pending_safe_command);
  dangerousCommandQueueFinish(g_pending_dangerous_command);
  invalidateDangerousCommandSession(g_dangerous_command_session);
  if (g_pairing_allowed_conn_hdl == conn_hdl) {
    g_pairing_allowed_conn_hdl = BLE_CONN_HANDLE_INVALID;
  }
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
  // Synchronous authorize so SoftDevice gets an immediate reply with live data.
  g_device_info.setReadAuthorizeCallback(onDeviceInfoRead, false);
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

  g_boot_ms = seed.boot_ms;
  g_pairing_window_started_ms = seed.boot_ms;
  g_pairing_window_ms = kDefaultPairingWindowMs;
  g_open_pairing_always = seed.open_pairing_always;
  g_pairing_allowed_conn_hdl = BLE_CONN_HANDLE_INVALID;
  g_config_valid = seed.config_from_flash;
  g_display_ok = seed.display_ok;
  g_fs_ok = seed.fs_ok;
  g_usb_connected = seed.usb_connected;
  g_deep_sleep_supported = false;

  g_device_info_packet = {};
  g_device_info_packet.struct_version = kBleStructVersion;
  g_device_info_packet.proto_major = kBleProtoMajor;
  g_device_info_packet.proto_minor = kBleProtoMinor;
  g_device_info_packet.hw_revision = static_cast<uint8_t>(HW_REVISION);
  strncpy(g_device_info_packet.model, "BIKECOMP-XIAO",
          sizeof(g_device_info_packet.model) - 1u);
  strncpy(g_device_info_packet.fw_version, FW_VERSION,
          sizeof(g_device_info_packet.fw_version) - 1u);
  memcpy(g_device_info_packet.serial, serial, 8);
  g_device_info_packet.reset_reason = seed.reset_reason;
  g_device_info_packet.boot_count = seed.boot_count;
  refreshDeviceInfoLiveFields(
      g_device_info_packet, g_boot_ms, seed.boot_ms, g_pairing_window_started_ms,
      g_pairing_window_ms,
      g_open_pairing_always, /*bonded=*/false, g_usb_connected, g_config_valid,
      g_display_ok, g_fs_ok, g_deep_sleep_supported);
  encodeAndPublishDeviceInfo();

  encodeConfiguration(config, g_config_read_buf);
  g_config_read.write(g_config_read_buf, kConfigurationSize);

  TelemetryPacket telemetry = {};
  telemetry.struct_version = kBleStructVersion;
  telemetry.last_pulse_age_ms = kTelemetryNoPulseAgeMs;
  telemetry.sensor_state = static_cast<uint8_t>(SensorState::kNoSignal);
  telemetry.battery_pct = seed.battery_percent;
  telemetry.seq = 0;
  encodeTelemetry(telemetry, g_telemetry_buf);
  g_telemetry.write(g_telemetry_buf, kTelemetrySize);
  g_telemetry_seq = 0;
  g_telemetry_has_published = false;
  g_sensor_test_active = false;

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
  g_serial_suffix = serial_suffix;
  resolveDeviceLocalName(config.device_name, serial_suffix, g_local_name,
                         sizeof(g_local_name));

  Bluefruit.configUuid128Count(8);
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  if (!Bluefruit.begin()) {
    return false;
  }

  Bluefruit.setEventCallback(onBleEvent);
  Bluefruit.Security.setPairCompleteCallback(onPairComplete);
  Bluefruit.setTxPower(4);
  Bluefruit.Periph.setDisconnectCallback(onDisconnect);
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

void BleManager::noteUsbPresent(bool usb_present) {
  g_usb_connected = usb_present;
}

void BleManager::setSensorTestActive(bool active) {
  g_sensor_test_active = active;
}

bool BleManager::sensorTestActive() const {
  return g_sensor_test_active;
}

void BleManager::openPairingWindow(uint16_t duration_s, uint32_t now_ms) {
  g_pairing_window_started_ms = now_ms;
  g_pairing_window_ms = static_cast<uint32_t>(duration_s) * 1000u;
}

void BleManager::clearBonds() {
  Bluefruit.Periph.clearBonds();
}

void BleManager::serviceTelemetry(const TelemetryBuildInput& input,
                                  uint32_t now_ms) {
  if (!ok_) return;

  const bool notify_enabled = g_telemetry.notifyEnabled();
  const TelemetryPublishMode mode =
      selectTelemetryPublishMode(notify_enabled, g_sensor_test_active);
  if (!telemetryDue(g_telemetry_last_publish_ms, now_ms, mode,
                    g_telemetry_has_published)) {
    return;
  }

  TelemetryPacket packet = {};
  fillTelemetryPacket(packet, input);
  g_telemetry_seq = nextTelemetrySeq(g_telemetry_seq);
  packet.seq = g_telemetry_seq;

  encodeTelemetry(packet, g_telemetry_buf);
  g_telemetry.write(g_telemetry_buf, kTelemetrySize);
  if (mode != TelemetryPublishMode::kUnsubscribed) {
    g_telemetry.notify(g_telemetry_buf, kTelemetrySize);
  }

  g_telemetry_last_publish_ms = now_ms;
  g_telemetry_has_published = true;
}

bool BleManager::hasPendingConfigWrite() const {
  return g_pending_config_write.state == ConfigWriteQueueState::kPending;
}

bool BleManager::takePendingConfigWrite(uint8_t out[kConfigurationSize]) {
  return configWriteQueueDequeue(g_pending_config_write, out);
}

void BleManager::completePendingConfigWrite() {
  configWriteQueueFinish(g_pending_config_write);
}

void BleManager::publishConfigWriteResult(const ConfigWriteResult& result) {
  publishCommandResult(kCommandResultConfigWriteId, result.status,
                       result.field_id);
}

void BleManager::publishAppliedConfig(const DeviceConfig& config,
                                      bool config_valid) {
  g_config_valid = config_valid;
  publishConfigRead(config);
  refreshAdvertisingName(config);
}

bool BleManager::hasPendingSafeCommand() const {
  return g_pending_safe_command.state == SafeCommandQueueState::kPending;
}

bool BleManager::takePendingSafeCommand(SafeCommandParseResult& command) {
  return safeCommandQueueDequeue(g_pending_safe_command, command);
}

void BleManager::completePendingSafeCommand() {
  safeCommandQueueFinish(g_pending_safe_command);
}

void BleManager::publishSafeCommandResult(CommandId command_id,
                                          const BleCommandResult& result) {
  publishCommandResult(static_cast<uint8_t>(command_id), result.status,
                       result.detail, result.token, result.payload,
                       result.payload_len);
}

bool BleManager::hasPendingDangerousCommand() const {
  return g_pending_dangerous_command.state ==
         DangerousCommandQueueState::kPending;
}

bool BleManager::takePendingDangerousCommand(
    DangerousCommandParseResult& command) {
  return dangerousCommandQueueDequeue(g_pending_dangerous_command, command);
}

void BleManager::completePendingDangerousCommand() {
  dangerousCommandQueueFinish(g_pending_dangerous_command);
}

void BleManager::publishDangerousCommandResult(
    CommandId command_id, const BleCommandResult& result) {
  publishCommandResult(static_cast<uint8_t>(command_id), result.status,
                       result.detail, result.token, result.payload,
                       result.payload_len);
}

}  // namespace bike
