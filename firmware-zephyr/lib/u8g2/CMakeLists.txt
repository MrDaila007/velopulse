# BikeComp u8g2 sources for Zephyr (same fonts as Arduino U8g2 2.35.x).

if(NOT DEFINED U8G2_ROOT)
  if(DEFINED ENV{U8G2_ROOT})
    set(U8G2_ROOT "$ENV{U8G2_ROOT}")
  elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../deps/u8g2/csrc/u8g2_font.c")
    set(U8G2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../deps/u8g2")
  elseif(EXISTS "/data/zephyrproject-v4.4/modules/u8g2")
    set(U8G2_ROOT "/data/zephyrproject-v4.4/modules/u8g2")
  else()
    message(FATAL_ERROR "Set U8G2_ROOT to the u8g2 source tree")
  endif()
endif()

set(U8G2_CSRC "${U8G2_ROOT}/csrc")
set(U8G2_FONTS_DIR "${U8G2_ROOT}/tools/font/build/single_font_files")

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/u8g2_bikecomp_fonts.c.in
  ${CMAKE_CURRENT_BINARY_DIR}/u8g2_bikecomp_fonts.c
  @ONLY
)

set(U8G2_BIKECOMP_SOURCES
  ${U8G2_CSRC}/u8g2_bitmap.c
  ${U8G2_CSRC}/u8g2_box.c
  ${U8G2_CSRC}/u8g2_buffer.c
  ${U8G2_CSRC}/u8g2_cleardisplay.c
  ${U8G2_CSRC}/u8g2_d_memory.c
  ${U8G2_CSRC}/u8g2_d_setup.c
  ${U8G2_CSRC}/u8g2_font.c
  ${U8G2_CSRC}/u8g2_hvline.c
  ${U8G2_CSRC}/u8g2_intersection.c
  ${U8G2_CSRC}/u8g2_ll_hvline.c
  ${U8G2_CSRC}/u8g2_setup.c
  ${U8G2_CSRC}/u8x8_8x8.c
  ${U8G2_CSRC}/u8x8_byte.c
  ${U8G2_CSRC}/u8x8_cad.c
  ${U8G2_CSRC}/u8x8_d_ssd1306_128x64_noname.c
  ${U8G2_CSRC}/u8x8_display.c
  ${U8G2_CSRC}/u8x8_gpio.c
  ${U8G2_CSRC}/u8x8_setup.c
  ${CMAKE_CURRENT_BINARY_DIR}/u8g2_bikecomp_fonts.c
  ${CMAKE_CURRENT_LIST_DIR}/u8g2_zephyr_port.cpp
)

target_sources(app PRIVATE ${U8G2_BIKECOMP_SOURCES})
target_include_directories(app PRIVATE ${U8G2_CSRC} ${CMAKE_CURRENT_LIST_DIR})
