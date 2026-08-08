# Shared domain source list for BikeComp Zephyr app and ztest builds.

set(BIKECOMP_FIRMWARE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../firmware")
set(BIKECOMP_DOMAIN_DIR "${BIKECOMP_FIRMWARE_ROOT}/lib/domain")

set(BIKECOMP_DOMAIN_SOURCES
  ${BIKECOMP_DOMAIN_DIR}/ambient_light_model.cpp
  ${BIKECOMP_DOMAIN_DIR}/battery_model.cpp
  ${BIKECOMP_DOMAIN_DIR}/ble_advertising.cpp
  ${BIKECOMP_DOMAIN_DIR}/ble_command.cpp
  ${BIKECOMP_DOMAIN_DIR}/ble_config_write.cpp
  ${BIKECOMP_DOMAIN_DIR}/ble_device_info.cpp
  ${BIKECOMP_DOMAIN_DIR}/ble_identity.cpp
  ${BIKECOMP_DOMAIN_DIR}/ble_telemetry.cpp
  ${BIKECOMP_DOMAIN_DIR}/boot_counter.cpp
  ${BIKECOMP_DOMAIN_DIR}/companion_snapshot.cpp
  ${BIKECOMP_DOMAIN_DIR}/config_codec.cpp
  ${BIKECOMP_DOMAIN_DIR}/config_validator.cpp
  ${BIKECOMP_DOMAIN_DIR}/load_config.cpp
  ${BIKECOMP_DOMAIN_DIR}/crc32.cpp
  ${BIKECOMP_DOMAIN_DIR}/diagnostics.cpp
  ${BIKECOMP_DOMAIN_DIR}/display_burn_in.cpp
  ${BIKECOMP_DOMAIN_DIR}/display_formatter.cpp
  ${BIKECOMP_DOMAIN_DIR}/display_layout.cpp
  ${BIKECOMP_DOMAIN_DIR}/display_power.cpp
  ${BIKECOMP_DOMAIN_DIR}/error_log.cpp
  ${BIKECOMP_DOMAIN_DIR}/odometer_save_policy.cpp
  ${BIKECOMP_DOMAIN_DIR}/page_carousel.cpp
  ${BIKECOMP_DOMAIN_DIR}/power_manager.cpp
  ${BIKECOMP_DOMAIN_DIR}/protocol_codec.cpp
  ${BIKECOMP_DOMAIN_DIR}/pulse_filter.cpp
  ${BIKECOMP_DOMAIN_DIR}/ride_state.cpp
  ${BIKECOMP_DOMAIN_DIR}/scheduler.cpp
  ${BIKECOMP_DOMAIN_DIR}/serial_console.cpp
  ${BIKECOMP_DOMAIN_DIR}/serial_profile.cpp
  ${BIKECOMP_DOMAIN_DIR}/speed_calculator.cpp
  ${BIKECOMP_DOMAIN_DIR}/speed_interval_guard.cpp
  ${BIKECOMP_DOMAIN_DIR}/storage_manager.cpp
  ${BIKECOMP_DOMAIN_DIR}/storage_migration.cpp
  ${BIKECOMP_DOMAIN_DIR}/trip_computer.cpp
)

set(BIKECOMP_DOMAIN_INCLUDES
  ${BIKECOMP_FIRMWARE_ROOT}/include
  ${BIKECOMP_DOMAIN_DIR}
)
