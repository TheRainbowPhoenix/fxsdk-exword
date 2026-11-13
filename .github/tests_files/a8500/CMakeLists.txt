cmake_minimum_required(VERSION 3.15)
project(ExwordA8500Test C ASM)

include(GenerateD01)
include(GenerateEXWORDPackage)

if(NOT DEFINED EXWORD_TEST_SOURCE OR NOT DEFINED LIBDATAPLUS_EX_SOURCE)
  message(FATAL_ERROR "Provide EXWORD_TEST_SOURCE and LIBDATAPLUS_EX_SOURCE")
endif()

set(EXWORD_FLAGS
  -mb -m4a-nofpu -Os -ffreestanding -fno-builtin
  -fno-pic -fno-pie -fno-stack-protector
  -fno-unwind-tables -fno-asynchronous-unwind-tables)

# Build only the actual libdataplus-ex graphics/input components used by
# exword-fxtst-TEST. No DEVKITSH4 or upstream libdataplus required.
add_library(exword_graphics STATIC
  "${LIBDATAPLUS_EX_SOURCE}/src/graphics/color.c"
  "${LIBDATAPLUS_EX_SOURCE}/src/graphics/drawing.c"
  "${LIBDATAPLUS_EX_SOURCE}/src/graphics/text.c"
  "${LIBDATAPLUS_EX_SOURCE}/src/graphics/lcdc.c")
add_library(exword_input STATIC
  "${LIBDATAPLUS_EX_SOURCE}/src/cpu/sh4a/input/keypad.c")
foreach(LIB exword_graphics exword_input)
  target_include_directories(${LIB} PUBLIC "${LIBDATAPLUS_EX_SOURCE}/include")
  target_compile_options(${LIB} PRIVATE ${EXWORD_FLAGS})
endforeach()

add_executable(fxtest
  "${CMAKE_CURRENT_SOURCE_DIR}/main.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/crt0.S")
target_compile_options(fxtest PRIVATE ${EXWORD_FLAGS})
target_link_options(fxtest PRIVATE
  -mb -m4a-nofpu -nostdlib -no-pie
  "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/exword.ld"
  -Wl,--build-id=none
  -Wl,--no-warn-rwx-segments)
set_property(TARGET fxtest APPEND PROPERTY
  LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exword.ld")
target_link_libraries(fxtest PRIVATE exword_graphics exword_input gcc)

generate_d01(TARGET fxtest OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/fxtest.d01"
  MODULE "fxtest")
generate_exword_package(TARGET fxtest
  D01 "${CMAKE_CURRENT_BINARY_DIR}/fxtest.d01"
  APPID FXTST TITLE "FxSDK bundle test!"
  HTML_DIR "${EXWORD_TEST_SOURCE}/html"
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/packages"
  LOCALES ja cn)
