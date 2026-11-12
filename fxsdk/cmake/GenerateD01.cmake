# Convert a linked EX-word ELF executable into a .d01 add-in.
# Requires the elf2d01 converter from https://github.com/brijohn/elf2d01.

function(generate_d01)
  cmake_parse_arguments(D01 "" "TARGET;OUTPUT;MODULE" "" ${ARGN})

  if(D01_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "generate_d01: Unknown arguments: ${D01_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT D01_TARGET)
    message(FATAL_ERROR "generate_d01: TARGET is required")
  endif()
  if(NOT TARGET "${D01_TARGET}")
    message(FATAL_ERROR "generate_d01: Unknown target '${D01_TARGET}'")
  endif()

  if(NOT D01_MODULE)
    set(D01_MODULE "cus000")
  endif()
  if(NOT D01_MODULE MATCHES "^[A-Za-z0-9_]{1,6}$")
    message(FATAL_ERROR "generate_d01: MODULE must be 1-6 ASCII letters, digits or underscores")
  endif()

  if(NOT D01_OUTPUT)
    set(D01_OUTPUT "${D01_TARGET}.d01")
  endif()
  get_filename_component(D01_OUTPUT "${D01_OUTPUT}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

  find_program(ELF2D01_EXECUTABLE NAMES elf2d01 REQUIRED)

  add_custom_command(TARGET "${D01_TARGET}" POST_BUILD
    COMMAND "${ELF2D01_EXECUTABLE}" -m "${D01_MODULE}"
      "$<TARGET_FILE:${D01_TARGET}>" "${D01_OUTPUT}"
    BYPRODUCTS "${D01_OUTPUT}"
    VERBATIM
    COMMENT "Generating EX-word add-in: ${D01_OUTPUT}")
endfunction()
