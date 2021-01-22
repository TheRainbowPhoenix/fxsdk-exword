# fxSDK toolchain file for Casio graphing calculators
# Models: Almost every model of the fx-9750G and fx-9860G families
# Target triplet: sh-elf (custom sh3eb-elf supporting sh3 and sh4-nofpu)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

set(FXSDK_PLATFORM fx)
set(FXSDK_PLATFORM_LONG fx9860G)

set(CMAKE_C_COMPILER sh-elf-gcc)
set(CMAKE_CXX_COMPILER sh-elf-g++)

add_compile_options(-mb -ffreestanding -nostdlib)
add_link_options(-nostdlib)
link_libraries(-lgcc)
add_compile_definitions(TARGET_FX9860G)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
