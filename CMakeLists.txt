# Build system for the fxSDK

cmake_minimum_required(VERSION 3.15)
project(fxSDK VERSION 2.8.0 LANGUAGES C)

option(FXLINK_DISABLE_UDISKS2 "Do not build the UDisks2-based features of fxlink")
option(FXLINK_DISABLE_SDL2 "Do not build the SDL2-based features of fxlink")

find_package(PkgConfig REQUIRED)
pkg_check_modules(libpng REQUIRED libpng16 IMPORTED_TARGET)
pkg_check_modules(libusb REQUIRED libusb-1.0 IMPORTED_TARGET)
# pkg_check_modules(libudev libudev IMPORTED_TARGET)
if(NOT FXLINK_DISABLE_UDISKS2)
  pkg_check_modules(udisks2 REQUIRED udisks2 IMPORTED_TARGET)
endif()
if(NOT FXLINK_DISABLE_SDL2)
  pkg_check_modules(sdl2 REQUIRED sdl2 IMPORTED_TARGET)
endif()

set(CMAKE_INSTALL_MESSAGE LAZY)
set(SRC "${CMAKE_CURRENT_SOURCE_DIR}")
set(BIN "${CMAKE_CURRENT_BINARY_DIR}")

add_compile_options(-Wall -Wextra -std=c11 -Og -g -D_GNU_SOURCE)

# fxgxa
add_executable(fxgxa fxgxa/dump.c fxgxa/edit.c fxgxa/file.c fxgxa/icon.c
  fxgxa/main.c fxgxa/util.c)
target_include_directories(fxgxa PUBLIC fxgxa/)
target_link_libraries(fxgxa PkgConfig::libpng)

# fxg1a as a symlink (for compatibility=
add_custom_target(fxg1a ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink "fxgxa" "fxg1a")

# fxsdk
add_custom_command(OUTPUT "${BIN}/fxsdk.sh"
  COMMAND sed
          -e "'s#@CMAKE_INSTALL_PREFIX@#${CMAKE_INSTALL_PREFIX}#'"
          -e "'s#@CMAKE_PROJECT_VERSION@#${CMAKE_PROJECT_VERSION}#'"
          "${SRC}/fxsdk/fxsdk.sh" > "${BIN}/fxsdk.sh"
  DEPENDS "${SRC}/fxsdk/fxsdk.sh")
add_custom_target(fxsdk ALL DEPENDS "${BIN}/fxsdk.sh")

# fxlink
configure_file(fxlink/config.h.in "${BIN}/include/fxlink/config.h")
add_executable(fxlink fxlink/usb.c fxlink/filter.c fxlink/interactive.c
  fxlink/main.c fxlink/png.c fxlink/properties.c fxlink/ud2.c fxlink/util.c
  fxlink/protocol.c fxlink/sdl2.c)
target_link_libraries(fxlink PkgConfig::libpng PkgConfig::libusb) # PkgConfig::libudev
target_include_directories(fxlink PRIVATE "${BIN}/include/fxlink")
if(NOT FXLINK_DISABLE_UDISKS2)
  target_link_libraries(fxlink PkgConfig::udisks2)
endif()
if(NOT FXLINK_DISABLE_SDL2)
  target_link_libraries(fxlink PkgConfig::sdl2)
endif()

# Install rules

# fxsdk
install(PROGRAMS "${BIN}/fxsdk.sh" TYPE BIN RENAME fxsdk)
install(DIRECTORY fxsdk/assets DESTINATION share/fxsdk)
install(DIRECTORY fxsdk/cmake/ DESTINATION lib/cmake/fxsdk)
# fxgxa, fxg1a
install(TARGETS fxgxa)
install(FILES "${BIN}/fxg1a" TYPE BIN)
# fxconv
install(PROGRAMS fxconv/fxconv-main.py TYPE BIN RENAME fxconv)
install(FILES fxconv/fxconv.py TYPE BIN)
#fxlink
install(TARGETS fxlink)
