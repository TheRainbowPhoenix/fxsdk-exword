cmake_minimum_required(VERSION 3.15)
project(ExwordTriangle C ASM)

include(GenerateD01)
include(GenerateUPDADN2)

set(COMMON_C_FLAGS
  -mb -m4a-nofpu -Os -Wall -Wextra
  -ffreestanding -fno-builtin -fno-stack-protector
  -fno-unwind-tables -fno-asynchronous-unwind-tables)
set(COMMON_LINK_FLAGS
  -mb -m4a-nofpu -nostdlib -Wl,--build-id=none -Wl,--no-warn-rwx-segments)

add_executable(triangle_d01 triangle.c d01/start.S)
target_compile_options(triangle_d01 PRIVATE ${COMMON_C_FLAGS})
target_link_options(triangle_d01 PRIVATE ${COMMON_LINK_FLAGS}
  "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/d01/link.ld")
set_property(TARGET triangle_d01 APPEND PROPERTY
  LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/d01/link.ld")
generate_d01(TARGET triangle_d01
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/triangle.d01" MODULE "triang")

add_executable(triangle_upd triangle.c updadn2/start.S)
target_compile_options(triangle_upd PRIVATE ${COMMON_C_FLAGS})
target_link_options(triangle_upd PRIVATE ${COMMON_LINK_FLAGS}
  "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/updadn2/link.ld")
set_property(TARGET triangle_upd APPEND PROPERTY
  LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/updadn2/link.ld")
generate_updadn2(TARGET triangle_upd
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/UPDADN2.BIN")
