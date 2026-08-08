#pragma once

#include "ble_protocol.h"

namespace bike {
namespace test_support {

DeviceInfoPacket makeNominalDeviceInfo();

TelemetryPacket makeMovingTelemetry();

TelemetryPacket makePausedTelemetry();

}  // namespace test_support
}  // namespace bike
