#pragma once

#include <stdint.h>

#include "u8g2.h"

namespace bike {

// Thin C++ wrapper matching the U8G2 API surface used by DisplayManager.
class U8g2Display {
 public:
  U8g2Display();
  bool begin();
  void setI2CAddress(uint8_t adr) { u8g2_SetI2CAddress(&u8g2_, adr); }
  void setContrast(uint8_t contrast) { u8g2_SetContrast(&u8g2_, contrast); }
  void setPowerSave(uint8_t is_enable) { u8g2_SetPowerSave(&u8g2_, is_enable); }
  void clearBuffer() { u8g2_ClearBuffer(&u8g2_); }
  void sendBuffer() { u8g2_SendBuffer(&u8g2_); }
  void setFont(const uint8_t* font) { u8g2_SetFont(&u8g2_, font); }
  void setDrawColor(uint8_t color) { u8g2_SetDrawColor(&u8g2_, color); }
  void drawStr(int16_t x, int16_t y, const char* text) {
    u8g2_DrawStr(&u8g2_, x, y, text);
  }
  void drawFrame(int16_t x, int16_t y, uint8_t w, uint8_t h) {
    u8g2_DrawFrame(&u8g2_, x, y, w, h);
  }
  void drawBox(int16_t x, int16_t y, uint8_t w, uint8_t h) {
    u8g2_DrawBox(&u8g2_, x, y, w, h);
  }

 private:
  u8g2_t u8g2_;
};

#if BIKECOMP_DISPLAY_HEIGHT == 64
using DisplayDriver = U8g2Display;
#else
using DisplayDriver = U8g2Display;
#endif

bool u8g2ZephyrI2cProbe(uint8_t address_7bit);

}  // namespace bike
