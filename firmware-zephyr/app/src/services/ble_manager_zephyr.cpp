#include "ble_manager.h"

#include <errno.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ble_advertising.h"
#include "ble_command.h"
#include "ble_config_write.h"
#include "ble_device_info.h"
#include "ble_identity.h"
#include "ble_protocol.h"
#include "ble_telemetry.h"
#include "companion_snapshot.h"
#include "error_log.h"
#include "platform.h"
#include "protocol_codec.h"

namespace bike {
namespace {

#define BIKECOMP_UUID_SERVICE_VAL \
  BT_UUID_128_ENCODE(0x7c9a0001, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
#define BIKECOMP_UUID_DEVICE_INFO_VAL \
  BT_UUID_128_ENCODE(0x7c9a0002, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
#define BIKECOMP_UUID_TELEMETRY_VAL \
  BT_UUID_128_ENCODE(0x7c9a0003, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
#define BIKECOMP_UUID_CONFIG_READ_VAL \
  BT_UUID_128_ENCODE(0x7c9a0004, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
#define BIKECOMP_UUID_CONFIG_WRITE_VAL \
  BT_UUID_128_ENCODE(0x7c9a0005, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
#define BIKECOMP_UUID_COMMAND_VAL \
  BT_UUID_128_ENCODE(0x7c9a0006, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
#define BIKECOMP_UUID_COMMAND_RESULT_VAL \
  BT_UUID_128_ENCODE(0x7c9a0007, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
#define BIKECOMP_UUID_ERROR_LOG_VAL \
  BT_UUID_128_ENCODE(0x7c9a0008, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
#define BIKECOMP_UUID_COMPANION_WRITE_VAL \
  BT_UUID_128_ENCODE(0x7c9a000b, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)

static const struct bt_uuid_128 bikecomp_service_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_SERVICE_VAL);
static const struct bt_uuid_128 device_info_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_DEVICE_INFO_VAL);
static const struct bt_uuid_128 telemetry_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_TELEMETRY_VAL);
static const struct bt_uuid_128 config_read_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_CONFIG_READ_VAL);
static const struct bt_uuid_128 config_write_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_CONFIG_WRITE_VAL);
static const struct bt_uuid_128 command_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_COMMAND_VAL);
static const struct bt_uuid_128 command_result_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_COMMAND_RESULT_VAL);
static const struct bt_uuid_128 error_log_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_ERROR_LOG_VAL);
static const struct bt_uuid_128 companion_write_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_COMPANION_WRITE_VAL);

enum BikecompAttrIndex {
  kAttrDeviceInfoVal = 2,
  kAttrTelemetryVal = 4,
  kAttrConfigReadVal = 7,
  kAttrConfigWriteVal = 10,
  kAttrCommandVal = 12,
  kAttrCommandResultVal = 14,
  kAttrErrorLogVal = 17,
};

uint8_t g_device_info_buf[kDeviceInfoSize] = {};
uint8_t g_telemetry_buf[kTelemetrySize] = {};
uint8_t g_config_read_buf[kConfigurationSize] = {};
uint8_t g_command_result_buf[kCommandResultMaxSize] = {};
uint8_t g_error_log_buf[kErrorLogMaxSize] = {};

char g_local_name[16] = {};
struct bt_conn* g_active_conn = nullptr;
bool g_advertising = false;
bool g_bt_ready = false;

DeviceInfoPacket g_device_info_packet = {};
uint32_t g_boot_ms = 0;
uint32_t g_pairing_window_started_ms = 0;
uint32_t g_pairing_window_ms = kDefaultPairingWindowMs;
bool g_open_pairing_always = false;
bool g_config_valid = false;
bool g_display_ok = false;
bool g_fs_ok = false;
bool g_usb_connected = false;
bool g_deep_sleep_supported = static_cast<bool>(IS_ENABLED(CONFIG_BIKECOMP_DEEP_SLEEP));
bool g_ble_always_advertise = true;
bool g_advertising_restart_pending = false;
bool g_aggressive_ble_power_save = false;

uint16_t g_telemetry_seq = 0;
uint32_t g_telemetry_last_publish_ms = 0;
bool g_telemetry_has_published = false;
bool g_telemetry_notify_enabled = false;
bool g_sensor_test_active = false;
uint16_t g_serial_suffix = 0;
constexpr uint16_t kInvalidConnBinding = 0xFFFFu;

uint16_t g_conn_binding_id = 0;
uint16_t g_pairing_allowed_conn_binding = kInvalidConnBinding;
PendingConfigWrite g_pending_config_write = {};

struct PendingCompanionWrite {
  enum class State : uint8_t { kIdle = 0, kPending };
  State state = State::kIdle;
  CompanionSnapshotPacket packet = {};
};

PendingCompanionWrite g_pending_companion_write = {};
PendingSafeCommand g_pending_safe_command = {};
PendingDangerousCommand g_pending_dangerous_command = {};
DangerousCommandSession g_dangerous_command_session = {};
ErrorLogBuffer g_error_log_entries;
bool g_error_log_dirty = false;

static struct k_sem g_bt_enable_sem;
static struct k_work_delayable g_adv_slow_work;
static struct k_work_delayable g_adv_idle_stop_work;
static bool g_adv_fast_phase = true;

static const struct bt_data g_ad_flags[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BIKECOMP_UUID_SERVICE_VAL),
};

static int8_t g_tx_power_dbm = 4;

static struct bt_le_adv_param g_adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN, BT_GAP_MS_TO_ADV_INTERVAL(kAdvertisingFastIntervalMs),
    BT_GAP_MS_TO_ADV_INTERVAL(kAdvertisingFastIntervalMs), nullptr);

void startAdvertising();
void stopAdvertising();
void scheduleAdvIdleStop();
void publishErrorLogIfDirty();
void publishCommandResult(uint8_t command_id, CommandStatus status,
                          uint8_t detail = 0, uint32_t token = 0,
                          const uint8_t* payload = nullptr,
                          uint8_t payload_len = 0);
void publishConfigRead(const DeviceConfig& config);
void buildScanResponse(struct bt_data* sd, size_t* count);

uint16_t connHandle(struct bt_conn* conn) {
  if (conn == nullptr || g_active_conn != conn) {
    return kInvalidConnBinding;
  }
  return g_conn_binding_id;
}

bool connectionBonded(struct bt_conn* conn) {
  if (conn == nullptr) {
    return false;
  }
  struct bt_conn_info info = {};
  if (bt_conn_get_info(conn, &info) != 0) {
    return false;
  }
  return info.type == BT_CONN_TYPE_LE && info.le.dst != nullptr &&
         bt_le_bond_exists(info.id, info.le.dst);
}

bool connectionTrusted(struct bt_conn* conn) {
  if (conn == nullptr) {
    return false;
  }
  struct bt_conn_info info = {};
  if (bt_conn_get_info(conn, &info) != 0) {
    return false;
  }
  return connectionBonded(conn) && info.security.level >= BT_SECURITY_L2;
}

uint32_t generateHardwareNonce() {
  uint32_t token = 0;
  if (sys_csrand_get(reinterpret_cast<uint8_t*>(&token), sizeof(token)) != 0) {
    return 0;
  }
  return token;
}

void readDeviceSerial(uint8_t serial[8], uint16_t& suffix_hex) {
  uint8_t hw_id[8] = {};
  const ssize_t len = hwinfo_get_device_id(hw_id, sizeof(hw_id));
  if (len >= 8) {
    memcpy(serial, hw_id, 8);
  } else if (len > 0) {
    memcpy(serial, hw_id, static_cast<size_t>(len));
  }
  suffix_hex = static_cast<uint16_t>(serial[0] | (static_cast<uint16_t>(serial[1]) << 8));
}

void refreshDeviceInfoForRead(struct bt_conn* conn) {
  refreshDeviceInfoLiveFields(
      g_device_info_packet, g_boot_ms, millis(), g_pairing_window_started_ms,
      g_pairing_window_ms, g_open_pairing_always, connectionBonded(conn),
      g_usb_connected, g_config_valid, g_display_ok, g_fs_ok,
      g_deep_sleep_supported);
  encodeDeviceInfo(g_device_info_packet, g_device_info_buf);
}

void stageErrorLog(ErrorLogCode code, ErrorLogSeverity severity, uint16_t detail,
                   uint32_t now_ms) {
  g_error_log_entries.append(static_cast<uint32_t>(now_ms - g_boot_ms) / 1000u,
                             code, severity, detail);
  g_error_log_dirty = true;
}

void refreshAdvertisingName(const DeviceConfig& config) {
  resolveDeviceLocalName(config.device_name, g_serial_suffix, g_local_name,
                         sizeof(g_local_name));
  bt_set_name(g_local_name);
}

void onConfigWrite(struct bt_conn* conn, const uint8_t* data, uint16_t len) {
  (void)conn;
  ConfigWriteRejectReason reject = ConfigWriteRejectReason::kNone;
  uint8_t* mutable_data = const_cast<uint8_t*>(data);
  if (!configWriteQueueStage(g_pending_config_write, mutable_data, len,
                             reject)) {
    const uint16_t detail = static_cast<uint16_t>(reject);
    if (reject == ConfigWriteRejectReason::kBusy) {
      stageErrorLog(ErrorLogCode::kConfigWriteRejected, ErrorLogSeverity::kWarn,
                    detail, millis());
      publishCommandResult(kCommandResultConfigWriteId, CommandStatus::kErrBusy);
      return;
    }
    stageErrorLog(ErrorLogCode::kConfigWriteRejected, ErrorLogSeverity::kWarn,
                  detail, millis());
    publishCommandResult(kCommandResultConfigWriteId, CommandStatus::kErrRange,
                         static_cast<uint8_t>(ConfigFieldId::kStructVersion));
  }
}

void onCompanionWrite(struct bt_conn* conn, const uint8_t* data, uint16_t len) {
  (void)conn;
  CompanionSnapshotPacket packet = {};
  if (!decodeCompanionSnapshot(data, len, packet)) return;
  g_pending_companion_write.packet = packet;
  g_pending_companion_write.state = PendingCompanionWrite::State::kPending;
}

void onCommandWrite(struct bt_conn* conn, const uint8_t* data, uint16_t len) {
  uint8_t command_id = 0;
  if (data != nullptr && len >= 2) {
    command_id = data[1];
  }
  const uint16_t conn_hdl = connHandle(conn);

  if (isDangerousCommandId(command_id)) {
    uint8_t* mutable_data = const_cast<uint8_t*>(data);
    const DangerousCommandParseResult parsed =
        parseDangerousBleCommand(mutable_data, len);
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
    const bool bonded = connectionTrusted(conn);
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

  uint8_t* mutable_data = const_cast<uint8_t*>(data);
  const SafeCommandParseResult parsed = parseSafeBleCommand(mutable_data, len);
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

ssize_t readDeviceInfo(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                       void* buf, uint16_t len, uint16_t offset) {
  (void)attr;
  refreshDeviceInfoForRead(conn);
  return bt_gatt_attr_read(conn, attr, buf, len, offset, g_device_info_buf,
                           kDeviceInfoSize);
}

ssize_t readFixedBuffer(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                        void* buf, uint16_t len, uint16_t offset) {
  const uint8_t* value = static_cast<const uint8_t*>(attr->user_data);
  const size_t value_len = attr->user_data == g_device_info_buf
                               ? kDeviceInfoSize
                               : (attr->user_data == g_telemetry_buf
                                      ? kTelemetrySize
                                      : kConfigurationSize);
  return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

ssize_t readVariableBuffer(struct bt_conn* conn,
                           const struct bt_gatt_attr* attr, void* buf,
                           uint16_t len, uint16_t offset) {
  const uint8_t* value = static_cast<const uint8_t*>(attr->user_data);
  const size_t value_len = attr->user_data == g_command_result_buf
                               ? sizeof(g_command_result_buf)
                               : sizeof(g_error_log_buf);
  return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

ssize_t writeConfig(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                    const void* buf, uint16_t len, uint16_t offset,
                    uint8_t flags) {
  (void)attr;
  (void)flags;
  if (offset != 0) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  if (len != kConfigurationSize) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
  }
  onConfigWrite(conn, static_cast<const uint8_t*>(buf), len);
  return len;
}

ssize_t writeCompanion(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                       const void* buf, uint16_t len, uint16_t offset,
                       uint8_t flags) {
  (void)attr;
  (void)flags;
  if (offset != 0) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  if (len != kCompanionSnapshotSize) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
  }
  onCompanionWrite(conn, static_cast<const uint8_t*>(buf), len);
  return len;
}

ssize_t writeCommand(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                     const void* buf, uint16_t len, uint16_t offset,
                     uint8_t flags) {
  (void)attr;
  (void)flags;
  if (offset != 0) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  if (len < kCommandMinSize || len > kCommandMaxSize) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
  }
  onCommandWrite(conn, static_cast<const uint8_t*>(buf), len);
  return len;
}

void telemetryCccChanged(const struct bt_gatt_attr* attr, uint16_t value) {
  (void)attr;
  g_telemetry_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

void advSlowWorkHandler(struct k_work* work) {
  (void)work;
  if (g_active_conn != nullptr || !g_advertising) {
    return;
  }
  g_adv_fast_phase = false;
  g_adv_param.interval_min = BT_GAP_MS_TO_ADV_INTERVAL(kAdvertisingSlowIntervalMs);
  g_adv_param.interval_max = BT_GAP_MS_TO_ADV_INTERVAL(kAdvertisingSlowIntervalMs);
  struct bt_data sd[2];
  size_t sd_count = 0;
  buildScanResponse(sd, &sd_count);
  bt_le_adv_stop();
  if (bt_le_adv_start(&g_adv_param, g_ad_flags, ARRAY_SIZE(g_ad_flags), sd,
                      sd_count) == 0) {
    g_advertising = true;
    scheduleAdvIdleStop();
  }
}

void advIdleStopWorkHandler(struct k_work* work) {
  (void)work;
  if (g_ble_always_advertise || g_active_conn != nullptr) {
    return;
  }
  stopAdvertising();
}

void scheduleAdvIdleStop() {
  k_work_cancel_delayable(&g_adv_idle_stop_work);
  if (g_ble_always_advertise) {
    return;
  }
  k_work_schedule(&g_adv_idle_stop_work,
                  K_SECONDS(advertisingTimeoutS(g_ble_always_advertise)));
}

void buildScanResponse(struct bt_data* sd, size_t* count) {
  sd[0] = BT_DATA(BT_DATA_NAME_COMPLETE, g_local_name,
                  strnlen(g_local_name, sizeof(g_local_name)));
  sd[1] = BT_DATA(BT_DATA_TX_POWER, &g_tx_power_dbm, sizeof(g_tx_power_dbm));
  *count = 2;
}

BT_GATT_SERVICE_DEFINE(
    bikecomp_svc, BT_GATT_PRIMARY_SERVICE(&bikecomp_service_uuid),
    BT_GATT_CHARACTERISTIC(&device_info_uuid.uuid, BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ, readDeviceInfo, nullptr,
                           g_device_info_buf),
    BT_GATT_CHARACTERISTIC(&telemetry_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, readFixedBuffer, nullptr,
                           g_telemetry_buf),
    BT_GATT_CCC(telemetryCccChanged, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&config_read_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, readFixedBuffer, nullptr,
                           g_config_read_buf),
    BT_GATT_CCC(nullptr, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_CHARACTERISTIC(&config_write_uuid.uuid, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_ENCRYPT, nullptr, writeConfig,
                           nullptr),
    BT_GATT_CHARACTERISTIC(&command_uuid.uuid, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_ENCRYPT, nullptr, writeCommand,
                           nullptr),
    BT_GATT_CHARACTERISTIC(
        &command_result_uuid.uuid,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
        readVariableBuffer, nullptr, g_command_result_buf),
    BT_GATT_CCC(nullptr, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_CHARACTERISTIC(&error_log_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, readVariableBuffer,
                           nullptr, g_error_log_buf),
    BT_GATT_CCC(nullptr, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_CHARACTERISTIC(&companion_write_uuid.uuid, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_ENCRYPT, nullptr, writeCompanion,
                           nullptr));

void publishErrorLogIfDirty() {
  if (!g_error_log_dirty) {
    return;
  }
  ErrorLogPacket packet = {};
  if (!g_error_log_entries.snapshot(packet)) {
    return;
  }
  const size_t length =
      encodeErrorLog(packet, g_error_log_buf, sizeof(g_error_log_buf));
  if (length == 0) {
    return;
  }
  bt_gatt_notify(g_active_conn, &bikecomp_svc.attrs[kAttrErrorLogVal],
                 g_error_log_buf, length);
  g_error_log_dirty = false;
}

void publishCommandResult(uint8_t command_id, CommandStatus status,
                          uint8_t detail, uint32_t token,
                          const uint8_t* payload, uint8_t payload_len) {
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
  const size_t encoded = encodeCommandResult(
      result, g_command_result_buf, sizeof(g_command_result_buf));
  if (encoded == 0) {
    return;
  }
  bt_gatt_notify(g_active_conn, &bikecomp_svc.attrs[kAttrCommandResultVal],
                 g_command_result_buf, encoded);
}

void publishConfigRead(const DeviceConfig& config) {
  encodeConfiguration(config, g_config_read_buf);
  bt_gatt_notify(g_active_conn, &bikecomp_svc.attrs[kAttrConfigReadVal],
                 g_config_read_buf, kConfigurationSize);
}

void stopAdvertising() {
  if (!g_advertising) {
    return;
  }
  bt_le_adv_stop();
  g_advertising = false;
  k_work_cancel_delayable(&g_adv_slow_work);
  k_work_cancel_delayable(&g_adv_idle_stop_work);
}

void startAdvertising() {
  if (g_active_conn != nullptr || g_advertising) {
    return;
  }
  g_adv_fast_phase = true;
  g_adv_param.interval_min = BT_GAP_MS_TO_ADV_INTERVAL(kAdvertisingFastIntervalMs);
  g_adv_param.interval_max = BT_GAP_MS_TO_ADV_INTERVAL(kAdvertisingFastIntervalMs);
  struct bt_data sd[2];
  size_t sd_count = 0;
  buildScanResponse(sd, &sd_count);
  const int err = bt_le_adv_start(&g_adv_param, g_ad_flags, ARRAY_SIZE(g_ad_flags),
                                  sd, sd_count);
  if (err == 0) {
    g_advertising = true;
    k_work_schedule(&g_adv_slow_work, K_SECONDS(kAdvertisingFastTimeoutS));
    scheduleAdvIdleStop();
  }
}

void connected(struct bt_conn* conn, uint8_t err) {
  if (err != 0) {
    return;
  }
  if (g_active_conn != nullptr) {
    bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    return;
  }
  g_active_conn = bt_conn_ref(conn);
  g_conn_binding_id++;
  stopAdvertising();
  const struct bt_le_conn_param param = BT_LE_CONN_PARAM_INIT(12, 48, 0, 400);
  bt_conn_le_param_update(conn, &param);
}

void disconnected(struct bt_conn* conn, uint8_t reason) {
  (void)reason;
  if (g_active_conn == conn) {
    bt_conn_unref(g_active_conn);
    g_active_conn = nullptr;
  }
  g_sensor_test_active = false;
  g_telemetry_notify_enabled = false;
  safeCommandQueueFinish(g_pending_safe_command);
  dangerousCommandQueueFinish(g_pending_dangerous_command);
  invalidateDangerousCommandSession(g_dangerous_command_session);
  g_pairing_allowed_conn_binding = kInvalidConnBinding;
  g_advertising_restart_pending = true;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

enum bt_security_err authPairingAccept(
    struct bt_conn* conn,
    const struct bt_conn_pairing_feat* const feat) {
  (void)feat;
  const uint16_t binding = connHandle(conn);
  if (shouldRejectPairingRequest(g_pairing_window_started_ms, millis(),
                                 g_pairing_window_ms, g_open_pairing_always,
                                 connectionBonded(conn))) {
    g_pairing_allowed_conn_binding = kInvalidConnBinding;
    stageErrorLog(ErrorLogCode::kPairingRejected, ErrorLogSeverity::kWarn,
                  binding, millis());
    return BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
  }
  g_pairing_allowed_conn_binding = binding;
  return BT_SECURITY_ERR_SUCCESS;
}

void authPairingConfirm(struct bt_conn* conn) {
  const uint16_t binding = connHandle(conn);
  if (shouldRejectPairingRequest(g_pairing_window_started_ms, millis(),
                                 g_pairing_window_ms, g_open_pairing_always,
                                 connectionBonded(conn))) {
    g_pairing_allowed_conn_binding = kInvalidConnBinding;
    stageErrorLog(ErrorLogCode::kPairingRejected, ErrorLogSeverity::kWarn,
                  binding, millis());
    bt_conn_auth_cancel(conn);
    return;
  }
  g_pairing_allowed_conn_binding = binding;
  bt_conn_auth_pairing_confirm(conn);
}

void authCancel(struct bt_conn* conn) {
  bt_conn_auth_cancel(conn);
  (void)conn;
}

void pairingComplete(struct bt_conn* conn, bool bonded) {
  const uint16_t binding = connHandle(conn);
  const bool was_allowed = g_pairing_allowed_conn_binding == binding;
  g_pairing_allowed_conn_binding = kInvalidConnBinding;
  if (!bonded || was_allowed) {
    return;
  }
  bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
  bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
}

static struct bt_conn_auth_cb auth_cb = {
    .pairing_accept = authPairingAccept,
    .cancel = authCancel,
    .pairing_confirm = authPairingConfirm,
};

static struct bt_conn_auth_info_cb auth_info_cb = {
    .pairing_complete = pairingComplete,
};

void btReady(int err) {
  if (err != 0) {
    g_bt_ready = false;
  } else {
    g_bt_ready = true;
  }
  k_sem_give(&g_bt_enable_sem);
}

bool seedGattBuffers(const DeviceConfig& config, const BleBootSeed& seed,
                     const uint8_t serial[8]) {
  g_error_log_entries.clear();
  g_error_log_dirty = false;

  g_boot_ms = seed.boot_ms;
  g_pairing_window_started_ms = seed.boot_ms;
  g_pairing_window_ms = kDefaultPairingWindowMs;
  g_open_pairing_always = seed.open_pairing_always;
  g_pairing_allowed_conn_binding = kInvalidConnBinding;
  g_config_valid = seed.config_from_flash;
  g_display_ok = seed.display_ok;
  g_fs_ok = seed.fs_ok;
  g_usb_connected = seed.usb_connected;
  g_deep_sleep_supported = static_cast<bool>(IS_ENABLED(CONFIG_BIKECOMP_DEEP_SLEEP));

  g_device_info_packet = {};
  g_device_info_packet.struct_version = kBleStructVersion;
  g_device_info_packet.proto_major = kBleProtoMajor;
  g_device_info_packet.proto_minor = kBleProtoMinor;
  g_device_info_packet.hw_revision = static_cast<uint8_t>(HW_REVISION);
  strncpy(g_device_info_packet.model, "BIKECOMP-XIAO",
          sizeof(g_device_info_packet.model) - 1u);
  strncpy(g_device_info_packet.fw_version, CONFIG_FW_VERSION,
          sizeof(g_device_info_packet.fw_version) - 1u);
  memcpy(g_device_info_packet.serial, serial, 8);
  g_device_info_packet.reset_reason = seed.reset_reason;
  g_device_info_packet.boot_count = seed.boot_count;
  refreshDeviceInfoLiveFields(
      g_device_info_packet, g_boot_ms, seed.boot_ms, g_pairing_window_started_ms,
      g_pairing_window_ms, g_open_pairing_always, /*bonded=*/false,
      g_usb_connected, g_config_valid, g_display_ok, g_fs_ok,
      g_deep_sleep_supported);
  encodeDeviceInfo(g_device_info_packet, g_device_info_buf);

  encodeConfiguration(config, g_config_read_buf);

  TelemetryPacket telemetry = {};
  telemetry.struct_version = kBleStructVersion;
  telemetry.last_pulse_age_ms = kTelemetryNoPulseAgeMs;
  telemetry.sensor_state = static_cast<uint8_t>(SensorState::kNoSignal);
  telemetry.battery_pct = seed.battery_percent;
  telemetry.seq = 0;
  encodeTelemetry(telemetry, g_telemetry_buf);
  g_telemetry_seq = 0;
  g_telemetry_has_published = false;
  g_sensor_test_active = false;

  CommandResultPacket idle = {};
  idle.struct_version = kBleStructVersion;
  idle.status = static_cast<uint8_t>(CommandStatus::kOk);
  encodeCommandResult(idle, g_command_result_buf, sizeof(g_command_result_buf));
  return true;
}

}  // namespace

bool BleManager::begin(const DeviceConfig& config, const BleBootSeed& seed) {
  ok_ = false;

  uint8_t serial[8] = {};
  uint16_t serial_suffix = 0;
  readDeviceSerial(serial, serial_suffix);
  g_serial_suffix = serial_suffix;
  resolveDeviceLocalName(config.device_name, serial_suffix, g_local_name,
                         sizeof(g_local_name));

  k_work_init_delayable(&g_adv_slow_work, advSlowWorkHandler);
  k_work_init_delayable(&g_adv_idle_stop_work, advIdleStopWorkHandler);

  int err = bt_enable(btReady);
  if (err != 0) {
    return false;
  }
  k_sem_take(&g_bt_enable_sem, K_FOREVER);
  if (!g_bt_ready) {
    return false;
  }

  if (IS_ENABLED(CONFIG_SETTINGS)) {
    settings_load();
  }

  bt_conn_auth_cb_register(&auth_cb);
  bt_conn_auth_info_cb_register(&auth_info_cb);

  if (!seedGattBuffers(config, seed, serial)) {
    return false;
  }

  g_ble_always_advertise = config.ble_always_advertise;
  g_advertising_restart_pending = false;
  bt_set_name(g_local_name);
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

bool BleManager::sensorTestActive() const { return g_sensor_test_active; }

void BleManager::openPairingWindow(uint16_t duration_s, uint32_t now_ms) {
  g_pairing_window_started_ms = now_ms;
  g_pairing_window_ms = static_cast<uint32_t>(duration_s) * 1000u;
}

void BleManager::noteMovement() {
  if (!ok_) {
    return;
  }
  if (shouldRestartAdvertisingOnMovement(g_active_conn != nullptr,
                                         g_advertising)) {
    startAdvertising();
  }
}

bool BleManager::statusLedActive() const {
  return bleAdvertising() || bleConnected();
}

bool BleManager::bleAdvertising() const {
  if (!ok_) {
    return false;
  }
  return g_advertising;
}

bool BleManager::bleConnected() const {
  if (!ok_) {
    return false;
  }
  return g_active_conn != nullptr;
}

void BleManager::stopAdvertising() {
  bike::stopAdvertising();  // calls the existing static free function in this TU
}

void BleManager::applyPowerSaveAdvertising(bool aggressive_power_save) {
  if (!ok_) return;
  const bool was_aggressive = g_aggressive_ble_power_save;
  g_aggressive_ble_power_save = aggressive_power_save;
  if (aggressive_power_save) {
    stopAdvertising();
    return;
  }
  if (was_aggressive && g_active_conn == nullptr) {
    startAdvertising();
  }
}

void BleManager::recordError(ErrorLogCode code, ErrorLogSeverity severity,
                             uint16_t detail, uint32_t now_ms) {
  if (!ok_) {
    return;
  }
  stageErrorLog(code, severity, detail, now_ms);
}

void BleManager::clearBonds() { bt_unpair(BT_ID_DEFAULT, nullptr); }

void BleManager::serviceTelemetry(const TelemetryBuildInput& input,
                                  uint32_t now_ms) {
  if (!ok_) {
    return;
  }
  if (g_advertising_restart_pending && g_active_conn == nullptr) {
    g_advertising_restart_pending = false;
    startAdvertising();
  }
  publishErrorLogIfDirty();

  const TelemetryPublishMode mode =
      selectTelemetryPublishMode(g_telemetry_notify_enabled,
                                 g_sensor_test_active);
  if (!telemetryDue(g_telemetry_last_publish_ms, now_ms, mode,
                    g_telemetry_has_published)) {
    return;
  }

  TelemetryPacket packet = {};
  fillTelemetryPacket(packet, input);
  g_telemetry_seq = nextTelemetrySeq(g_telemetry_seq);
  packet.seq = g_telemetry_seq;

  encodeTelemetry(packet, g_telemetry_buf);
  if (mode != TelemetryPublishMode::kUnsubscribed) {
    bt_gatt_notify(g_active_conn, &bikecomp_svc.attrs[kAttrTelemetryVal],
                   g_telemetry_buf, kTelemetrySize);
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

bool BleManager::hasPendingCompanionWrite() const {
  return g_pending_companion_write.state ==
         PendingCompanionWrite::State::kPending;
}

bool BleManager::takePendingCompanionWrite(CompanionSnapshotPacket& out) {
  if (!hasPendingCompanionWrite()) return false;
  out = g_pending_companion_write.packet;
  g_pending_companion_write.state = PendingCompanionWrite::State::kIdle;
  return true;
}

void BleManager::publishConfigWriteResult(const ConfigWriteResult& result) {
  publishCommandResult(kCommandResultConfigWriteId, result.status,
                       result.field_id);
}

void BleManager::publishAppliedConfig(const DeviceConfig& config,
                                      bool config_valid) {
  g_config_valid = config_valid;
  publishConfigRead(config);
  const bool disconnected = g_active_conn == nullptr;
  if (disconnected) {
    stopAdvertising();
  }
  refreshAdvertisingName(config);
  g_ble_always_advertise = config.ble_always_advertise;
  if (disconnected) {
    startAdvertising();
  }
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

void BleManager::publishDangerousCommandResult(CommandId command_id,
                                               const BleCommandResult& result) {
  publishCommandResult(static_cast<uint8_t>(command_id), result.status,
                       result.detail, result.token, result.payload,
                       result.payload_len);
}

void BleManager::serviceCsc(uint32_t now_ms) { (void)now_ms; }

CscSnapshot BleManager::cscSnapshot(uint32_t now_ms) const {
  (void)now_ms;
  return {};
}

void BleManager::startCscPairing(uint16_t duration_s, uint32_t now_ms) {
  (void)duration_s;
  (void)now_ms;
}

void BleManager::forgetCscBond() {}

bool BleManager::takePendingCscBond(CscBondData& out) {
  (void)out;
  return false;
}

CscWheelDelta BleManager::takeCscWheelDelta() { return {}; }

bool BleManager::cscWheelSpeedSourceActive(uint32_t now_ms) const {
  (void)now_ms;
  return false;
}

}  // namespace bike
