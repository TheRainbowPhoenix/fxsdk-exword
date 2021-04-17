function(generate_g1a)
  cmake_parse_arguments(G1A "" "TARGET;OUTPUT;NAME;INTERNAL;VERSION;DATE;ICON" "" ${ARGN})

  # Check arguments

  if(DEFINED G1A_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "generate_g1a: Unrecognized arguments ${G1A_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT DEFINED G1A_TARGET)
    message(FATAL_ERROR "generate_g1a: TARGET argument is required")
  endif()

  # Find output file name

  if(DEFINED G1A_OUTPUT)
    get_filename_component(G1A_OUTPUT "${G1A_OUTPUT}" ABSOLUTE
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    set(G1A_OUTPUT "${G1A_TARGET}.g1a")
  endif()

  # Compute the set of fxg1a arguments

  set(FXG1A_ARGS "")

  # Support empty names enen though they're not normally used in g1a files
  if(DEFINED G1A_NAME OR "NAME" IN_LIST G1A_KEYWORDS_MISSING_VALUES)
    list(APPEND FXG1A_ARGS "-n" "${G1A_NAME}")
  endif()

  if(DEFINED G1A_ICON)
    get_filename_component(G1A_ICON "${G1A_ICON}" ABSOLUTE
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    list(APPEND FXG1A_ARGS "-i" "${G1A_ICON}")
  endif()

  string(REGEX REPLACE "sh-elf-gcc$" "sh-elf-objcopy" OBJCOPY "${CMAKE_C_COMPILER}")

  add_custom_command(
    TARGET "${G1A_TARGET}" POST_BUILD
    COMMAND "${OBJCOPY}" -O binary -R .bss -R .gint_bss "${G1A_TARGET}" "${G1A_TARGET}.bin"
    COMMAND fxg1a ${FXG1A_ARGS} -o "${G1A_OUTPUT}" "${G1A_TARGET}.bin"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  )
  if(DEFINED G1A_ICON)
    set_target_properties("${G1A_TARGET}" PROPERTIES LINK_DEPENDS "${G1A_ICON}")
  endif()
endfunction()
