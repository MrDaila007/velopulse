#include "ble_advertising.h"

namespace bike {

uint16_t advertisingTimeoutS(bool always_advertise) {
  return always_advertise ? 0u : kAdvertisingIdleTimeoutS;
}

bool shouldRestartAdvertisingOnMovement(bool connected,
                                        bool advertising_running) {
  return !connected && !advertising_running;
}

}  // namespace bike
