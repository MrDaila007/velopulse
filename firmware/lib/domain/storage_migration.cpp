#include "storage_migration.h"

#include "config_codec.h"

namespace bike {

size_t configPayloadLengthForVersion(uint16_t version) {
  if (version == kConfigRecordVersionV1 || version == kConfigRecordVersion) {
    return kDeviceConfigPayloadSize;
  }
  return 0;
}

size_t odometerPayloadLengthForVersion(uint16_t version) {
  if (version == kOdometerRecordVersionV1 ||
      version == kOdometerRecordVersion) {
    return kOdometerPayloadSize;
  }
  return 0;
}

bool migrateConfigV1ToV2(const uint8_t* payload_v1,
                         size_t length,
                         DeviceConfig& config) {
  return decodeDeviceConfig(payload_v1, length, config);
}

bool migrateOdometerV1ToV2(const uint8_t* payload_v1,
                           size_t length,
                           OdometerData& odometer) {
  return decodeOdometer(payload_v1, length, odometer);
}

bool migrateConfigToCurrent(uint16_t from_version,
                            const uint8_t* payload,
                            size_t length,
                            DeviceConfig& config) {
  if (payload == nullptr) return false;
  if (from_version == kConfigRecordVersion) {
    return decodeDeviceConfig(payload, length, config);
  }
  if (from_version == kConfigRecordVersionV1 &&
      kConfigRecordVersion == 2) {
    return migrateConfigV1ToV2(payload, length, config);
  }
  return false;
}

bool migrateOdometerToCurrent(uint16_t from_version,
                              const uint8_t* payload,
                              size_t length,
                              OdometerData& odometer) {
  if (payload == nullptr) return false;
  if (from_version == kOdometerRecordVersion) {
    return decodeOdometer(payload, length, odometer);
  }
  if (from_version == kOdometerRecordVersionV1 &&
      kOdometerRecordVersion == 2) {
    return migrateOdometerV1ToV2(payload, length, odometer);
  }
  return false;
}

}  // namespace bike
