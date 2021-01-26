# fxSDK toolchain file for Casio graphing calculators
# Models: Prizm fx-CG 10, fx-CG 20, fx-CG 50, fx-CG 50 emulator
# Target triplet: sh-elf (custom sh3eb-elf supporting sh3 and sh4-nofpu)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

set(FXSDK_PLATFORM cg)
set(FXSDK_PLATFORM_LONG fxCG50)

set(CMAKE_C_COMPILER sh-elf-gcc)
set(CMAKE_CXX_COMPILER sh-elf-g++)

add_compile_options(-m4-nofpu -mb -ffreestanding -nostdlib -Wa,--dsp)
add_link_options(-nostdlib)
link_libraries(-lgcc)
add_compile_definitions(TARGET_FXCG50)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Determine compiler install path
execute_process(
  COMMAND ${CMAKE_C_COMPILER} --print-search-dirs
  COMMAND grep install
  COMMAND sed "s/install: //"
  OUTPUT_VARIABLE FXSDK_COMPILER_INSTALL
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
