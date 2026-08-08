#pragma once

#include <stdint.h>

namespace bike {

void suppressBoardLeds();
void beginBoardLeds();
void restoreBoardChargeIndicator();
void updateBoardStatusLed(bool ble_advertising, bool ble_connected,
                          uint32_t now_ms);

}  // namespace bike
