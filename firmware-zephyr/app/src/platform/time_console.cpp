#include "platform.h"

#include <stdio.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

namespace {

constexpr uint32_t kMicrosWrapUs = 0xFFFFFFFFu;

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
#if defined(CONFIG_USB_DEVICE_STACK)
  // VBUS detect is board-specific; nRF52840 USBREG is not exposed in Zephyr yet.
  // Treat as unknown/false until a board hook is added.
  return false;
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
  ready_ = true;
}

int SerialConsole::available() { return 0; }

int SerialConsole::read() { return -1; }

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
