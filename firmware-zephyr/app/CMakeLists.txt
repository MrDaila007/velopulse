cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bikecomp_zephyr)

set(FIRMWARE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware)
set(DOMAIN_DIR ${FIRMWARE_ROOT}/lib/domain)

set(DOMAIN_SOURCES
  ${DOMAIN_DIR}/ambient_light_model.cpp
  ${DOMAIN_DIR}/battery_model.cpp
  ${DOMAIN_DIR}/ble_advertising.cpp
  ${DOMAIN_DIR}/ble_command.cpp
  ${DOMAIN_DIR}/ble_config_write.cpp
  ${DOMAIN_DIR}/ble_device_info.cpp
  ${DOMAIN_DIR}/ble_identity.cpp
  ${DOMAIN_DIR}/ble_telemetry.cpp
  ${DOMAIN_DIR}/boot_counter.cpp
  ${DOMAIN_DIR}/config_codec.cpp
  ${DOMAIN_DIR}/config_validator.cpp
  ${DOMAIN_DIR}/crc32.cpp
  ${DOMAIN_DIR}/diagnostics.cpp
  ${DOMAIN_DIR}/display_burn_in.cpp
  ${DOMAIN_DIR}/display_formatter.cpp
  ${DOMAIN_DIR}/display_layout.cpp
  ${DOMAIN_DIR}/display_power.cpp
  ${DOMAIN_DIR}/error_log.cpp
  ${DOMAIN_DIR}/odometer_save_policy.cpp
  ${DOMAIN_DIR}/page_carousel.cpp
  ${DOMAIN_DIR}/protocol_codec.cpp
  ${DOMAIN_DIR}/pulse_filter.cpp
  ${DOMAIN_DIR}/ride_state.cpp
  ${DOMAIN_DIR}/scheduler.cpp
  ${DOMAIN_DIR}/serial_console.cpp
  ${DOMAIN_DIR}/speed_calculator.cpp
  ${DOMAIN_DIR}/speed_interval_guard.cpp
  ${DOMAIN_DIR}/storage_manager.cpp
  ${DOMAIN_DIR}/storage_migration.cpp
  ${DOMAIN_DIR}/trip_computer.cpp
)

set(PLATFORM_SOURCES
  src/platform/time_console.cpp
  src/platform/adc_io.cpp
  src/platform/littlefs_backend.cpp
)

set(SERVICE_SOURCES
  src/services/wheel_sensor.cpp
  src/services/battery_manager.cpp
  src/services/ambient_light_manager.cpp
  src/services/board_leds.cpp
  src/smoke/zephyr_smoke.cpp
)

if(CONFIG_BIKECOMP_ZEPHYR_BLE_STUB)
  list(APPEND SERVICE_SOURCES src/services/ble_manager_stub.cpp)
else()
  list(APPEND SERVICE_SOURCES src/services/ble_manager_zephyr.cpp)
endif()

if(CONFIG_BIKECOMP_ZEPHYR_DISPLAY_STUB)
  list(APPEND SERVICE_SOURCES src/services/display_manager_stub.cpp)
  target_compile_definitions(app PRIVATE BIKECOMP_ZEPHYR_DISPLAY_STUB=1)
else()
  list(APPEND SERVICE_SOURCES src/services/display_manager.cpp)
endif()

target_sources(app PRIVATE
  src/main.cpp
  src/app_controller.cpp
  ${PLATFORM_SOURCES}
  ${SERVICE_SOURCES}
  ${DOMAIN_SOURCES}
)

target_include_directories(app PRIVATE
  include
  src
  src/platform
  src/services
  src/smoke
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/u8g2
  ${FIRMWARE_ROOT}/include
  ${DOMAIN_DIR}
)

target_compile_definitions(app PRIVATE
  BOARD_SUPER_NRF52840=1
  BIKECOMP_HALL_TWO_WIRE=1
  BIKECOMP_SUPPRESS_BOARD_LEDS=1
  BIKECOMP_AMBIENT_RAW_DARK=100
  BIKECOMP_AMBIENT_RAW_BRIGHT=3900
  BIKECOMP_DISPLAY_HEIGHT=${CONFIG_BIKECOMP_DISPLAY_HEIGHT}
  PROTO_MAJOR=1
  PROTO_MINOR=1
  HW_REVISION=1
)

if(CONFIG_BIKECOMP_AMBIENT_LIGHT)
  target_compile_definitions(app PRIVATE BIKECOMP_AMBIENT_LIGHT=1)
else()
  target_compile_definitions(app PRIVATE BIKECOMP_AMBIENT_LIGHT=0)
endif()

if(CONFIG_BIKECOMP_HOTPATH_SERIAL)
  target_compile_definitions(app PRIVATE BIKECOMP_HOTPATH_SERIAL=1)
else()
  target_compile_definitions(app PRIVATE BIKECOMP_HOTPATH_SERIAL=0)
endif()

if(CONFIG_BIKECOMP_OPEN_PAIRING)
  target_compile_definitions(app PRIVATE BIKECOMP_OPEN_PAIRING=1)
else()
  target_compile_definitions(app PRIVATE BIKECOMP_OPEN_PAIRING=0)
endif()

target_compile_options(app PRIVATE
  -Wall
  -Wextra
  -Werror=return-type
  -Wno-unused-parameter
)

if(NOT CONFIG_BIKECOMP_ZEPHYR_DISPLAY_STUB)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/u8g2 lib/u8g2)
endif()
