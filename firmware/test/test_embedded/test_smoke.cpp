#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "board_pins.h"
#include "config.h"

void setUp() {}
void tearDown() {}

void test_display_is_present() {
  Wire.begin();
  Wire.beginTransmission(bike::kDisplayI2cAddress);
  TEST_ASSERT_EQUAL_UINT8(0, Wire.endTransmission());
}

void setup() {
  delay(1500);
  UNITY_BEGIN();
  RUN_TEST(test_display_is_present);
  UNITY_END();
}

void loop() {}
