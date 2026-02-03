include(FxsdkUtils)

function(generate_hh3)
  cmake_parse_arguments(HH3 "" "TARGET;OUTPUT" "" ${ARGN})

  # Check arguments

  if(DEFINED HH3_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "generate_hh3: Unrecognized arguments ${HH3_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT DEFINED HH3_TARGET)
    message(FATAL_ERROR "generate_hh3: TARGET argument is required")
  endif()

  # Find output file name

  if(DEFINED HH3_OUTPUT)
    get_filename_component(HH3_OUTPUT "${HH3_OUTPUT}" ABSOLUTE
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    shell_escape("${HH3_OUTPUT}" HH3_OUTPUT)
  else()
    set(HH3_OUTPUT "${HH3_TARGET}.hh3")
  endif()

  # Generate strip command

  string(REPLACE "gcc" "strip" STRIP "${CMAKE_C_COMPILER}")

  add_custom_command(
    TARGET "${HH3_TARGET}" POST_BUILD
    COMMAND ${STRIP} -s -o ${HH3_OUTPUT} ${HH3_TARGET}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND_EXPAND_LISTS)
endfunction()
