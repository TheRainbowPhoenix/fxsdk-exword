# fxSDK toolchain file for old Casio EXWord dictionaries
# Models: XD-Axxxx, XD-Dxxxx and other Dataplus 5-6 series.
# Target triplet: sh-elf (custom sh3eb-elf supporting sh3 and sh4-nofpu)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR sh)

set(FXSDK_PLATFORM exword)
set(FXSDK_PLATFORM_LONG exword)

set(FXSDK_TOOLCHAIN sh-elf-)
set(CMAKE_C_COMPILER sh-elf-gcc)
set(CMAKE_CXX_COMPILER sh-elf-g++)
set(CMAKE_ASM_COMPILER sh-elf-gcc)

set(CMAKE_C_FLAGS_INIT "")
set(CMAKE_CXX_FLAGS_INIT "")

# No -mexword: use the regular SH4 ABI and a dedicated EX-word linker script.
add_compile_options(-mb -m4a-nofpu -ffreestanding -fno-builtin)
add_link_options(-mb -m4a-nofpu -nostdlib -Wl,--build-id=none)
add_compile_definitions(TARGET_EXWORD)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(FXSDK_CMAKE_MODULE_PATH "${FXSDK_CMAKE_MODULE_PATH}")

set(FXSDK_PREFIX "$ENV{FXSDK_PREFIX}")
foreach(DIR IN LISTS FXSDK_PREFIX)
  include_directories("${DIR}/include")
  link_directories("${DIR}/lib")
endforeach()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --print-file-name=.
  OUTPUT_VARIABLE FXSDK_COMPILER_INSTALL
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND fxsdk path sysroot
  OUTPUT_VARIABLE FXSDK_SYSROOT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND fxsdk path include
  OUTPUT_VARIABLE FXSDK_INCLUDE
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND fxsdk path lib
  OUTPUT_VARIABLE FXSDK_LIB
  OUTPUT_STRIP_TRAILING_WHITESPACE)
