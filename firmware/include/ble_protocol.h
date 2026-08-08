#pragma once

// Mirror of protocol/data-structures.md and protocol/uuids.md (protocol v1.1).
// Wire layouts are packed little-endian; Configuration encoding goes through
// DeviceConfig + config_codec (single source of truth for the 48-byte config).

#include <stddef.h>
#include <stdint.h>

#include "config_codec.h"
#include "types.h"

namespace bike {

// ---------------------------------------------------------------------------
// UUIDs (text form, RFC 4122). Binary GATT registration is BleManager's job.
// ---------------------------------------------------------------------------

constexpr char kBleServiceUuid[] =
    "7C9A0001-4B7D-4F2E-9C1A-2E6D5F8B31A4";
constexpr char kBleDeviceInfoUuid[] =
    "7C9A0002-4B7D-4F2E-9C1A-2E6D5F8B31A4";
constexpr char kBleTelemetryUuid[] =
    "7C9A0003-4B7D-4F2E-9C1A-2E6D5F8B31A4";
constexpr char kBleConfigReadUuid[] =
    "7C9A0004-4B7D-4F2E-9C1A-2E6D5F8B31A4";
constexpr char kBleConfigWriteUuid[] =
    "7C9A0005-4B7D-4F2E-9C1A-2E6D5F8B31A4";
constexpr char kBleCommandUuid[] =
    "7C9A0006-4B7D-4F2E-9C1A-2E6D5F8B31A4";
constexpr char kBleCommandResultUuid[] =
    "7C9A0007-4B7D-4F2E-9C1A-2E6D5F8B31A4";
constexpr char kBleErrorLogUuid[] =
    "7C9A0008-4B7D-4F2E-9C1A-2E6D5F8B31A4";
constexpr char kBleCompanionWriteUuid[] =
    "7C9A000B-4B7D-4F2E-9C1A-2E6D5F8B31A4";

constexpr uint8_t kBleStructVersion = 1;
constexpr uint8_t kBleProtoMajor = 1;
constexpr uint8_t kBleProtoMinor = 1;

constexpr size_t kDeviceInfoSize = 48;
constexpr size_t kTelemetrySize = 36;
constexpr size_t kConfigurationSize = kDeviceConfigPayloadSize;  // 48
constexpr size_t kCommandHeaderSize = 4;
constexpr size_t kCommandMaxPayload = 16;
constexpr size_t kCommandMinSize = kCommandHeaderSize;
constexpr size_t kCommandMaxSize = kCommandHeaderSize + kCommandMaxPayload;
constexpr size_t kCommandResultHeaderSize = 9;
constexpr size_t kCommandResultMaxPayload = 16;
constexpr size_t kCommandResultMinSize = kCommandResultHeaderSize;
constexpr size_t kCommandResultMaxSize =
    kCommandResultHeaderSize + kCommandResultMaxPayload;
constexpr size_t kErrorLogEntrySize = 8;
constexpr size_t kErrorLogMaxEntries = 4;
constexpr size_t kErrorLogHeaderSize = 2;
constexpr size_t kErrorLogMinSize = kErrorLogHeaderSize + kErrorLogEntrySize;
constexpr size_t kErrorLogMaxSize =
    kErrorLogHeaderSize + kErrorLogMaxEntries * kErrorLogEntrySize;

constexpr uint8_t kCommandResultConfigWriteId = 0xF0;

// ---------------------------------------------------------------------------
// Enumerations (protocol §8)
// ---------------------------------------------------------------------------

enum class ResetReason : uint8_t {
  kUnknown = 0,
  kPowerOn = 1,
  kPinReset = 2,
  kWatchdog = 3,
  kSoftReset = 4,
  kLockup = 5,
  kWakeFromSleep = 6,
  kBrownout = 7,
};

enum class SensorState : uint8_t {
  kOk = 0,
  kIdle = 1,
  kStuck = 2,
  kNoSignal = 3,
};

enum class PowerState : uint8_t {
  kActive = 0,
  kShortStop = 1,
  kIdleDisplayOff = 2,
  kDeepSleepPending = 3,
  kBleConfig = 4,
  kCharging = 5,
};

enum class CommandId : uint8_t {
  kResetTrip = 0x01,
  kResetMaxSpeed = 0x02,
  kForceSave = 0x03,
  kDisplayOn = 0x04,
  kDisplayOff = 0x05,
  kDisplayTest = 0x06,
  kSensorTestStart = 0x07,
  kSensorTestStop = 0x08,
  kBatteryTest = 0x09,
  kStartDiagnostic = 0x0A,
  kGetDiagnostic = 0x0B,
  kResetOdometer = 0x20,
  kFactoryReset = 0x21,
  kReboot = 0x22,
  kSetBatteryCal = 0x23,
  kSetOdometer = 0x30,
  kOpenPairingWindow = 0x40,
};

enum class CommandStatus : uint8_t {
  kOk = 0,
  kErrUnknownCommand = 1,
  kErrLength = 2,
  kErrStructVersion = 3,
  kErrRange = 4,
  kNeedsConfirm = 5,
  kErrTokenInvalid = 6,
  kErrTokenExpired = 7,
  kErrNotPaired = 8,
  kErrBusy = 9,
  kErrStorage = 10,
  kErrHardware = 11,
  kErrNotSupported = 12,
};

enum class ErrorLogSeverity : uint8_t {
  kInfo = 0,
  kWarn = 1,
  kError = 2,
};

enum class ErrorLogCode : uint8_t {
  kI2cTimeout = 0x01,
  kFlashError = 0x02,
  kConfigCrc = 0x03,
  kIsrOverflow = 0x04,
  kSensorStuck = 0x05,
  kWatchdogReset = 0x06,
  kCriticalBattery = 0x07,
  kConfigWriteRejected = 0x08,
  kPairingRejected = 0x09,
};

// Configuration field_id values equal wire offsets (protocol §8.6).
enum class ConfigFieldId : uint8_t {
  kStructVersion = 0,
  kFlags = 1,
  kWheelCircumferenceMm = 2,
  kMaxSpeedKmh = 4,
  kStopTimeoutS = 5,
  kDisplayTimeoutS = 6,
  kDeepSleepTimeoutS = 8,
  kBrightnessPct = 10,
  kPageSwitchPeriodS = 11,
  kEnabledPagesMask = 12,
  kLowBatteryPct = 13,
  kOdometerSaveIntervalM = 14,
  kSmoothingWindow = 16,
  kDebounceMs = 17,
  kActiveEdge = 18,
  kPinnedPage = 19,
  kBattCalScalePermille = 20,
  kBattCalOffsetMv = 22,
  kPageOrder = 24,
  kDeviceName = 30,
};

// Device Information flags
constexpr uint8_t kDeviceInfoFlagConfigValid = 1u << 0;
constexpr uint8_t kDeviceInfoFlagDisplayOk = 1u << 1;
constexpr uint8_t kDeviceInfoFlagFsOk = 1u << 2;
constexpr uint8_t kDeviceInfoFlagBonded = 1u << 3;
constexpr uint8_t kDeviceInfoFlagPairingWindowOpen = 1u << 4;
constexpr uint8_t kDeviceInfoFlagUsbConnected = 1u << 5;
constexpr uint8_t kDeviceInfoFlagDeepSleepSupported = 1u << 6;

// Telemetry flags
constexpr uint8_t kTelemetryFlagMoving = 1u << 0;
constexpr uint8_t kTelemetryFlagDisplayOn = 1u << 1;
constexpr uint8_t kTelemetryFlagCharging = 1u << 2;
constexpr uint8_t kTelemetryFlagUsbConnected = 1u << 3;
constexpr uint8_t kTelemetryFlagLowBattery = 1u << 4;
constexpr uint8_t kTelemetryFlagSmoothingEnabled = 1u << 5;
constexpr uint8_t kTelemetryFlagUnitsImperial = 1u << 6;
constexpr uint8_t kTelemetryFlagChargeStatusUnknown = 1u << 7;

// Command flags
constexpr uint8_t kCommandFlagHasToken = 1u << 0;

constexpr uint32_t kTelemetryNoPulseAgeMs = 0xFFFFFFFFu;

#pragma pack(push, 1)

struct DeviceInfoPacket {
  uint8_t struct_version;
  uint8_t proto_major;
  uint8_t proto_minor;
  uint8_t hw_revision;
  char model[16];
  char fw_version[12];
  uint8_t serial[8];
  uint32_t uptime_s;
  uint8_t reset_reason;
  uint16_t boot_count;
  uint8_t flags;
};

struct TelemetryPacket {
  uint8_t struct_version;
  uint8_t flags;
  uint16_t speed_x100;
  uint16_t avg_speed_x100;
  uint16_t max_speed_x100;
  uint32_t trip_distance_cm;
  uint32_t moving_time_s;
  uint32_t odometer_m;
  uint16_t battery_mv;
  uint8_t battery_pct;
  uint8_t ride_state;
  uint32_t revolutions;
  uint32_t last_pulse_age_ms;
  uint16_t seq;
  uint8_t sensor_state;
  uint8_t power_state;
};

// Wire layout only — encode/decode via DeviceConfig + config_codec.
struct ConfigurationPacket {
  uint8_t struct_version;
  uint8_t flags;
  uint16_t wheel_circumference_mm;
  uint8_t max_speed_kmh;
  uint8_t stop_timeout_s;
  uint16_t display_timeout_s;
  uint16_t deep_sleep_timeout_s;
  uint8_t brightness_pct;
  uint8_t page_switch_period_s;
  uint8_t enabled_pages_mask;
  uint8_t low_battery_pct;
  uint16_t odometer_save_interval_m;
  uint8_t smoothing_window;
  uint8_t debounce_ms;
  uint8_t active_edge;
  uint8_t pinned_page;
  uint16_t batt_cal_scale_permille;
  int16_t batt_cal_offset_mv;
  uint8_t page_order[5];
  uint8_t reserved_page;
  char device_name[16];
  uint16_t reserved;
};

struct CommandPacket {
  uint8_t struct_version;
  uint8_t command_id;
  uint8_t flags;
  uint8_t payload_len;
  uint8_t payload[kCommandMaxPayload];
};

struct CommandResultPacket {
  uint8_t struct_version;
  uint8_t command_id;
  uint8_t status;
  uint8_t detail;
  uint32_t token;
  uint8_t payload_len;
  uint8_t payload[kCommandResultMaxPayload];
};

struct ErrorLogEntry {
  uint32_t uptime_s;
  uint8_t code;
  uint8_t severity;
  uint16_t detail;
};

struct ErrorLogPacket {
  uint8_t struct_version;
  uint8_t entry_count;
  ErrorLogEntry entries[kErrorLogMaxEntries];
};

#pragma pack(pop)

static_assert(sizeof(DeviceInfoPacket) == kDeviceInfoSize,
              "DeviceInfo layout mismatch");
static_assert(sizeof(TelemetryPacket) == kTelemetrySize,
              "Telemetry layout mismatch");
static_assert(sizeof(ConfigurationPacket) == kConfigurationSize,
              "Configuration layout mismatch");
static_assert(sizeof(CommandPacket) == kCommandMaxSize,
              "Command layout mismatch");
static_assert(sizeof(CommandResultPacket) == kCommandResultMaxSize,
              "CommandResult layout mismatch");
static_assert(sizeof(ErrorLogEntry) == kErrorLogEntrySize,
              "ErrorLogEntry layout mismatch");
// GET_DIAGNOSTIC 16-byte payload: DiagnosticSnapshot + encodeDiagnosticPayload
// in diagnostics.h (owned with storage counters export).

static_assert(offsetof(DeviceInfoPacket, model) == 4, "DeviceInfo.model");
static_assert(offsetof(DeviceInfoPacket, serial) == 32, "DeviceInfo.serial");
static_assert(offsetof(DeviceInfoPacket, uptime_s) == 40, "DeviceInfo.uptime");
static_assert(offsetof(TelemetryPacket, trip_distance_cm) == 8,
              "Telemetry.trip_distance_cm");
static_assert(offsetof(TelemetryPacket, seq) == 32, "Telemetry.seq");
static_assert(offsetof(ConfigurationPacket, wheel_circumference_mm) == 2,
              "Config.wheel_circumference_mm");
static_assert(offsetof(ConfigurationPacket, page_order) == 24,
              "Config.page_order");
static_assert(offsetof(ConfigurationPacket, device_name) == 30,
              "Config.device_name");
static_assert(kConfigurationSize == 48, "Config size must stay 48");
static_assert(static_cast<uint8_t>(RideState::kIdle) == 0, "RideState.IDLE");
static_assert(static_cast<uint8_t>(RideState::kMoving) == 1, "RideState.MOVING");
static_assert(static_cast<uint8_t>(RideState::kPaused) == 2, "RideState.PAUSED");

}  // namespace bike
