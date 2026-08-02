#include "zephyr_smoke.h"

#include "platform.h"

namespace bike {

void runZephyrSmokeChecks(const ZephyrSmokeContext& context) {
  Serial.println("Zephyr smoke:");
  Serial.print("  fs_mounted=");
  Serial.println(context.fs_mounted ? "OK" : "FAIL");
  Serial.print("  adc_valid=");
  Serial.println(context.adc_valid ? "OK" : "FAIL");
  Serial.print("  hall_configured=");
  Serial.println(context.hall_configured ? "OK" : "FAIL");
  Serial.print("  selftest_mask=0x");
  if (context.selftest_mask < 0x10u) Serial.print('0');
  Serial.println(context.selftest_mask, HEX);
  Serial.print("  free_heap=");
  Serial.println(platform::freeHeapBytes());
}

}  // namespace bike
