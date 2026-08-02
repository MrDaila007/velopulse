cmake_minimum_required(VERSION 3.20.0)

project(bikecomp_domain_tests)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/domain_sources.cmake)

set(TEST_SHARED_DIR ${BIKECOMP_FIRMWARE_ROOT}/test/shared)

target_sources(
  testbinary
  PRIVATE
    src/test_codec_storage.cpp
    src/test_motion.cpp
    src/test_protocol.cpp
    ${TEST_SHARED_DIR}/fixture_loader.cpp
    ${TEST_SHARED_DIR}/memory_storage_backend.cpp
    ${TEST_SHARED_DIR}/protocol_samples.cpp
    ${BIKECOMP_DOMAIN_SOURCES}
)

target_include_directories(testbinary PRIVATE ${BIKECOMP_DOMAIN_INCLUDES}
                                              ${TEST_SHARED_DIR})

target_compile_definitions(
  testbinary
  PRIVATE PROTOCOL_FIXTURES_DIR="${CMAKE_CURRENT_LIST_DIR}/../../../protocol/fixtures"
          PROTO_MAJOR=1
          PROTO_MINOR=0
          HW_REVISION=1)

target_compile_options(testbinary PRIVATE -Wall -Wextra -Wno-unused-parameter)
