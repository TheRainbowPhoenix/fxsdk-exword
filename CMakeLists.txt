# Build system for the fxSDK

cmake_minimum_required(VERSION 3.16)
project(fxSDK VERSION 2.3.1 LANGUAGES C)

option(FXLINK_DISABLE_UDISKS2 "Do not build the UDisks2-based features of fxlink")

find_package(PkgConfig REQUIRED)
find_package(PNG REQUIRED)
pkg_check_modules(libusb REQUIRED libusb-1.0 IMPORTED_TARGET)
# pkg_check_modules(libudev libudev IMPORTED_TARGET)
if(NOT FXLINK_DISABLE_UDISKS2)
  pkg_check_modules(udisks2 REQUIRED udisks2 IMPORTED_TARGET)
endif()

set(CMAKE_INSTALL_MESSAGE LAZY)
set(SRC "${CMAKE_CURRENT_SOURCE_DIR}")
set(BIN "${CMAKE_CURRENT_BINARY_DIR}")

add_compile_options(-Wall -Wextra -std=c11 -Og -g -D_GNU_SOURCE)

# fxg1a
add_executable(fxg1a fxg1a/dump.c fxg1a/edit.c fxg1a/file.c fxg1a/icon.c
  fxg1a/main.c fxg1a/util.c)
target_include_directories(fxg1a PUBLIC fxg1a/)
target_link_libraries(fxg1a PNG::PNG)

# fxsdk
add_custom_command(OUTPUT "${BIN}/fxsdk.sh"
  COMMAND sed "'s#@FXSDK_PREFIX@#${CMAKE_INSTALL_PREFIX}#'"
          "${SRC}/fxsdk/fxsdk.sh" > "${BIN}/fxsdk.sh"
  DEPENDS "${SRC}/fxsdk/fxsdk.sh")
add_custom_target(fxsdk ALL DEPENDS "${BIN}/fxsdk.sh")

# fxlink
configure_file(fxlink/config.h.in "${BIN}/include/fxlink/config.h")
add_executable(fxlink fxlink/usb.c fxlink/filter.c fxlink/main.c
  fxlink/properties.c fxlink/ud2.c fxlink/util.c)
target_link_libraries(fxlink PkgConfig::libusb) # PkgConfig::libudev
target_include_directories(fxlink PRIVATE "${BIN}/include/fxlink")
if(NOT FXLINK_DISABLE_UDISKS2)
  target_link_libraries(fxlink PkgConfig::udisks2)
endif()

# Install rules

# fxsdk
install(PROGRAMS "${BIN}/fxsdk.sh" TYPE BIN RENAME fxsdk)
install(DIRECTORY fxsdk/assets DESTINATION share/fxsdk)
install(DIRECTORY fxsdk/cmake/ DESTINATION lib/cmake/fxsdk)
# fxg1a
install(TARGETS fxg1a)
# fxconv
install(PROGRAMS fxconv/fxconv-main.py TYPE BIN RENAME fxconv)
install(FILES fxconv/fxconv.py TYPE BIN)
#fxlink
install(TARGETS fxlink)
