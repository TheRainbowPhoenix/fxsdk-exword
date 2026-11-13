# Convert an already linked updater ELF to the raw UPDADN2.BIN format.
# The 128-byte updater header must be supplied by the ELF's .header section.

function(generate_updadn2)
  cmake_parse_arguments(UPD "" "TARGET;OUTPUT" "" ${ARGN})
  if(UPD_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "generate_updadn2: Unexpected arguments: ${UPD_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT UPD_TARGET OR NOT TARGET "${UPD_TARGET}")
    message(FATAL_ERROR "generate_updadn2: TARGET must name an existing target")
  endif()
  if(NOT UPD_OUTPUT)
    set(UPD_OUTPUT "UPDADN2.BIN")
  endif()
  get_filename_component(UPD_OUTPUT "${UPD_OUTPUT}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  if(NOT CMAKE_OBJCOPY)
    find_program(CMAKE_OBJCOPY NAMES sh-elf-objcopy REQUIRED)
  endif()
  add_custom_command(TARGET "${UPD_TARGET}" POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O binary
      "$<TARGET_FILE:${UPD_TARGET}>" "${UPD_OUTPUT}"
    BYPRODUCTS "${UPD_OUTPUT}"
    VERBATIM
    COMMENT "Generating EX-word updater image: ${UPD_OUTPUT}")
endfunction()
