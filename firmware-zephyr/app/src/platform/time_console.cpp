#include "platform.h"

#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/ring_buffer.h>

#if defined(CONFIG_SOC_SERIES_NRF52X)
#include <hal/nrf_power.h>
#endif

namespace {

constexpr size_t kSerialRxRingSize = 256;

ring_buf rx_ring_;
uint8_t rx_ring_buf_[kSerialRxRingSize];

#if DT_HAS_CHOSEN(zephyr_console)
const struct device* const kUartDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
#else
const struct device* const kUartDev = nullptr;
#endif

void uartRxCallback(const struct device* dev, void* user_data) {
  (void)user_data;
  if (!uart_irq_update(dev)) return;
  if (!uart_irq_rx_ready(dev)) return;

  uint8_t byte = 0;
  while (uart_fifo_read(dev, &byte, 1) == 1) {
    ring_buf_put(&rx_ring_, &byte, 1);
  }
}

#if defined(CONFIG_SOC_SERIES_NRF52X) && DT_HAS_CHOSEN(zephyr_console)
// Adafruit nRF52 bootloader: host opens CDC at 1200 baud then drops DTR.
constexpr uint32_t kDfuMagicSerialOnlyReset = 0x4e;

struct BootloaderTouchWork {
  struct k_work_delayable work;
  bool last_dtr = false;
};

BootloaderTouchWork bootloader_touch_;

void checkBootloaderTouch(struct k_work* work) {
  auto* touch = CONTAINER_OF(work, BootloaderTouchWork, work.work);
  if (kUartDev == nullptr || !device_is_ready(kUartDev)) return;

  uint32_t dtr = 0;
  uint32_t baud = 0;
  if (uart_line_ctrl_get(kUartDev, UART_LINE_CTRL_DTR, &dtr) != 0) return;
  if (uart_line_ctrl_get(kUartDev, UART_LINE_CTRL_BAUD_RATE, &baud) != 0) return;

  const bool dtr_active = dtr != 0;
  if (touch->last_dtr && !dtr_active && baud == 1200) {
    nrf_power_gpregret_set(NRF_POWER, kDfuMagicSerialOnlyReset);
    sys_reboot(SYS_REBOOT_COLD);
    return;
  }
  touch->last_dtr = dtr_active;

  (void)k_work_reschedule(&touch->work, K_MSEC(50));
}

void initBootloaderTouch() {
  k_work_init_delayable(&bootloader_touch_.work, checkBootloaderTouch);
  (void)k_work_schedule(&bootloader_touch_.work, K_MSEC(200));
}
#endif

void initSerialRx() {
#if DT_HAS_CHOSEN(zephyr_console)
  if (kUartDev == nullptr || !device_is_ready(kUartDev)) return;
  ring_buf_init(&rx_ring_, sizeof(rx_ring_buf_), rx_ring_buf_);
  uart_irq_callback_user_data_set(kUartDev, uartRxCallback, nullptr);
  uart_irq_rx_enable(kUartDev);
#if defined(CONFIG_SOC_SERIES_NRF52X)
  initBootloaderTouch();
#endif
#endif
}

}  // namespace

namespace bike {
namespace platform {

uint32_t millis() { return k_uptime_get_32(); }

uint32_t micros() {
  const uint64_t cycles = k_cycle_get_64();
  const uint64_t hz = sys_clock_hw_cycles_per_sec();
  return static_cast<uint32_t>((cycles * 1000000ull) / hz);
}

void yield() { k_yield(); }

uint32_t resetReasonRaw() {
  uint32_t cause = 0;
  if (hwinfo_get_reset_cause(&cause) != 0) return 0;
  return cause;
}

void clearResetReason(uint32_t raw) {
  (void)raw;
  hwinfo_clear_reset_cause();
}

int freeHeapBytes() { return 0; }

bool usbPresent() {
#if defined(CONFIG_SOC_SERIES_NRF52X)
  return nrf_power_usbregstatus_vbusdet_get(NRF_POWER) != 0;
#else
  return false;
#endif
}

void systemReset() { sys_reboot(SYS_REBOOT_COLD); }

}  // namespace platform
}  // namespace bike

SerialConsole Serial;

void SerialConsole::begin(unsigned long baud) {
  (void)baud;
  initSerialRx();
#if DT_HAS_CHOSEN(zephyr_console)
  ready_ = (kUartDev != nullptr && device_is_ready(kUartDev));
#else
  ready_ = true;
#endif
}

int SerialConsole::available() {
  return static_cast<int>(ring_buf_size_get(&rx_ring_));
}

int SerialConsole::read() {
  uint8_t byte = 0;
  if (ring_buf_get(&rx_ring_, &byte, 1) != 1) return -1;
  return static_cast<int>(byte);
}

void SerialConsole::print(const char* text) { printk("%s", text); }

void SerialConsole::print(char value) { printk("%c", value); }

void SerialConsole::print(unsigned value) { printk("%u", value); }

void SerialConsole::print(int value) { printk("%d", value); }

void SerialConsole::print(uint16_t value) { printk("%u", value); }

void SerialConsole::print(uint64_t value) { printk("%llu", value); }

void SerialConsole::print(unsigned value, int base) {
  if (base == 16) {
    printk("%X", value);
  } else {
    printk("%u", value);
  }
}

void SerialConsole::println() { printk("\n"); }

void SerialConsole::println(const char* text) {
  printk("%s\n", text);
}

void SerialConsole::println(int value) { printk("%d\n", value); }

void SerialConsole::println(unsigned value) { printk("%u\n", value); }

void SerialConsole::println(uint16_t value) { printk("%u\n", value); }

void SerialConsole::println(int16_t value) { printk("%d\n", value); }

void SerialConsole::println(char value) { printk("%c\n", value); }

void SerialConsole::println(bool value) { printk("%d\n", value ? 1 : 0); }

void SerialConsole::println(unsigned value, int base) {
  if (base == 16) {
    printk("%X\n", value);
  } else {
    printk("%u\n", value);
  }
}
