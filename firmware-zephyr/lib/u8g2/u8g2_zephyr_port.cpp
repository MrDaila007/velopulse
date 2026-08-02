#include "u8g2_display.hpp"

extern "C" {
#include "u8g2.h"
}

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define OLED_I2C_NODE DT_NODELABEL(i2c1)

namespace bike {
namespace {

constexpr size_t kI2cTxBufferSize = 32;

const struct device* i2cBus() {
#if DT_NODE_HAS_STATUS(OLED_I2C_NODE, okay)
  const struct device* dev = DEVICE_DT_GET(OLED_I2C_NODE);
  return device_is_ready(dev) ? dev : nullptr;
#else
  return nullptr;
#endif
}

}  // namespace

extern "C" uint8_t u8x8_byte_zephyr_hw_i2c(u8x8_t* u8x8, uint8_t msg,
                                             uint8_t arg_int, void* arg_ptr) {
  static uint8_t buffer[kI2cTxBufferSize];
  static uint8_t buf_idx = 0;

  const struct device* bus = i2cBus();
  switch (msg) {
    case U8X8_MSG_BYTE_SEND: {
      auto* data = static_cast<uint8_t*>(arg_ptr);
      while (arg_int-- > 0 && buf_idx < kI2cTxBufferSize) {
        buffer[buf_idx++] = *data++;
      }
      break;
    }
    case U8X8_MSG_BYTE_INIT:
      break;
    case U8X8_MSG_BYTE_SET_DC:
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      buf_idx = 0;
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      if (bus == nullptr) return 0;
      return i2c_write(bus, reinterpret_cast<uint8_t*>(buffer), buf_idx,
                         u8x8_GetI2CAddress(u8x8) >> 1) == 0;
    default:
      return 0;
  }
  return 1;
}

extern "C" uint8_t u8x8_gpio_and_delay_zephyr(u8x8_t* u8x8, uint8_t msg,
                                              uint8_t arg_int, void* arg_ptr) {
  (void)u8x8;
  (void)arg_ptr;
  switch (msg) {
    case U8X8_MSG_DELAY_MILLI:
      k_msleep(arg_int);
      break;
    case U8X8_MSG_DELAY_10MICRO:
      k_busy_wait(arg_int * 10u);
      break;
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
    case U8X8_MSG_DELAY_NANO:
    case U8X8_MSG_DELAY_100NANO:
      break;
    default:
      break;
  }
  return 1;
}

U8g2Display::U8g2Display() {
#if BIKECOMP_DISPLAY_HEIGHT == 64
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2_, U8G2_R0, u8x8_byte_zephyr_hw_i2c,
                                         u8x8_gpio_and_delay_zephyr);
#else
  u8g2_Setup_ssd1306_i2c_128x32_univision_f(&u8g2_, U8G2_R0,
                                            u8x8_byte_zephyr_hw_i2c,
                                            u8x8_gpio_and_delay_zephyr);
#endif
}

bool U8g2Display::begin() {
  u8g2_InitDisplay(&u8g2_);
  u8g2_ClearBuffer(&u8g2_);
  u8g2_SetPowerSave(&u8g2_, 0);
  return true;
}

bool u8g2ZephyrI2cProbe(uint8_t address_7bit) {
  const struct device* bus = i2cBus();
  if (bus == nullptr) return false;
  return i2c_write(bus, nullptr, 0, address_7bit) == 0;
}

}  // namespace bike
