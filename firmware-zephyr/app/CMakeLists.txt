cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bikecomp_zephyr)

set(FIRMWARE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/domain_sources.cmake)
set(DOMAIN_DIR ${BIKECOMP_DOMAIN_DIR})
set(DOMAIN_SOURCES ${BIKECOMP_DOMAIN_SOURCES})

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
