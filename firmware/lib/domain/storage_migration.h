#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "storage_manager.h"

namespace bike {

// Flash record format versions (RecordHeader.version). Distinct from BLE
// DeviceConfig.struct_version, which remains 1 while the 48-byte layout is
// unchanged. Bumping the record version exercises the migration path on load.
constexpr uint16_t kConfigRecordVersionV1 = 1;
constexpr uint16_t kOdometerRecordVersionV1 = 1;

size_t configPayloadLengthForVersion(uint16_t version);
size_t odometerPayloadLengthForVersion(uint16_t version);

// Identity migration stub: v1 payload layout matches current DeviceConfig.
// Future schema changes replace the body and keep the hook name for chaining.
bool migrateConfigV1ToV2(const uint8_t* payload_v1,
                         size_t length,
                         DeviceConfig& config);

bool migrateOdometerV1ToV2(const uint8_t* payload_v1,
                           size_t length,
                           OdometerData& odometer);

bool migrateConfigToCurrent(uint16_t from_version,
                            const uint8_t* payload,
                            size_t length,
                            DeviceConfig& config);

bool migrateOdometerToCurrent(uint16_t from_version,
                              const uint8_t* payload,
                              size_t length,
                              OdometerData& odometer);

}  // namespace bike
