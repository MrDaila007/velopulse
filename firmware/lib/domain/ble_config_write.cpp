#include "ble_config_write.h"

#include <string.h>

namespace bike {

uint8_t configValidationErrorToFieldId(ConfigValidationError error) {
  switch (error) {
    case ConfigValidationError::kWheelCircumference:
      return static_cast<uint8_t>(ConfigFieldId::kWheelCircumferenceMm);
    case ConfigValidationError::kMaxSpeed:
      return static_cast<uint8_t>(ConfigFieldId::kMaxSpeedKmh);
    case ConfigValidationError::kStopTimeout:
      return static_cast<uint8_t>(ConfigFieldId::kStopTimeoutS);
    case ConfigValidationError::kDisplayTimeout:
      return static_cast<uint8_t>(ConfigFieldId::kDisplayTimeoutS);
    case ConfigValidationError::kDeepSleepTimeout:
      return static_cast<uint8_t>(ConfigFieldId::kDeepSleepTimeoutS);
    case ConfigValidationError::kBrightness:
      return static_cast<uint8_t>(ConfigFieldId::kBrightnessPct);
    case ConfigValidationError::kPageSwitchPeriod:
      return static_cast<uint8_t>(ConfigFieldId::kPageSwitchPeriodS);
    case ConfigValidationError::kEnabledPagesMask:
      return static_cast<uint8_t>(ConfigFieldId::kEnabledPagesMask);
    case ConfigValidationError::kLowBattery:
      return static_cast<uint8_t>(ConfigFieldId::kLowBatteryPct);
    case ConfigValidationError::kOdometerSaveInterval:
      return static_cast<uint8_t>(ConfigFieldId::kOdometerSaveIntervalM);
    case ConfigValidationError::kSmoothingWindow:
      return static_cast<uint8_t>(ConfigFieldId::kSmoothingWindow);
    case ConfigValidationError::kDebounce:
      return static_cast<uint8_t>(ConfigFieldId::kDebounceMs);
    case ConfigValidationError::kActiveEdge:
      return static_cast<uint8_t>(ConfigFieldId::kActiveEdge);
    case ConfigValidationError::kPinnedPage:
      return static_cast<uint8_t>(ConfigFieldId::kPinnedPage);
    case ConfigValidationError::kBatteryScale:
      return static_cast<uint8_t>(ConfigFieldId::kBattCalScalePermille);
    case ConfigValidationError::kBatteryOffset:
      return static_cast<uint8_t>(ConfigFieldId::kBattCalOffsetMv);
    case ConfigValidationError::kPageOrder:
      return static_cast<uint8_t>(ConfigFieldId::kPageOrder);
    case ConfigValidationError::kDeviceName:
      return static_cast<uint8_t>(ConfigFieldId::kDeviceName);
    case ConfigValidationError::kNone:
    default:
      return 0;
  }
}

uint8_t configWireErrorToFieldId(DeviceConfigWireError error) {
  switch (error) {
    case DeviceConfigWireError::kBadVersion:
      return static_cast<uint8_t>(ConfigFieldId::kStructVersion);
    case DeviceConfigWireError::kBadPadding:
      return static_cast<uint8_t>(ConfigFieldId::kPageOrder);
    case DeviceConfigWireError::kBadLength:
    case DeviceConfigWireError::kOk:
    default:
      return static_cast<uint8_t>(ConfigFieldId::kStructVersion);
  }
}

ConfigWriteParseResult parseConfigWritePayload(const uint8_t* data, size_t length) {
  ConfigWriteParseResult result;
  if (data == nullptr || length != kConfigurationSize) {
    result.status = CommandStatus::kErrRange;
    result.field_id = static_cast<uint8_t>(ConfigFieldId::kStructVersion);
    return result;
  }

  DeviceConfig decoded = {};
  const DeviceConfigWireError wire_error =
      tryDecodeDeviceConfigWire(data, length, decoded);
  if (wire_error != DeviceConfigWireError::kOk) {
    result.status = CommandStatus::kErrRange;
    result.field_id = configWireErrorToFieldId(wire_error);
    return result;
  }

  const ConfigValidationError validation_error =
      ConfigValidator::validate(decoded);
  if (validation_error != ConfigValidationError::kNone) {
    result.status = CommandStatus::kErrRange;
    result.field_id = configValidationErrorToFieldId(validation_error);
    return result;
  }

  result.ok = true;
  result.config = decoded;
  return result;
}

bool configWriteQueueStage(PendingConfigWrite& queue,
                             const uint8_t* data,
                             size_t length,
                             ConfigWriteRejectReason& reject) {
  reject = ConfigWriteRejectReason::kNone;
  if (queue.state != ConfigWriteQueueState::kIdle) {
    reject = ConfigWriteRejectReason::kBusy;
    return false;
  }
  if (data == nullptr || length != kConfigurationSize) {
    reject = ConfigWriteRejectReason::kBadLength;
    return false;
  }
  memcpy(queue.data, data, kConfigurationSize);
  queue.state = ConfigWriteQueueState::kPending;
  return true;
}

bool configWriteQueueDequeue(PendingConfigWrite& queue,
                            uint8_t out[kConfigurationSize]) {
  if (queue.state != ConfigWriteQueueState::kPending || out == nullptr) {
    return false;
  }
  memcpy(out, queue.data, kConfigurationSize);
  return true;
}

void configWriteQueueFinish(PendingConfigWrite& queue) {
  queue.state = ConfigWriteQueueState::kIdle;
}

}  // namespace bike
