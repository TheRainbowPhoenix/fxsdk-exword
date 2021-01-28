function(find_simple_library _library _version_header _version_macro)
  cmake_parse_arguments(CLV "" "PATH_VAR;VERSION_VAR" "" ${ARGN})

  # Find the library path
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} "-print-file-name=${_library}"
    OUTPUT_VARIABLE LIB_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)

  if("${LIB_PATH}" STREQUAL "${_library}")
    message("lib not found")
    return()
  endif()

  if(DEFINED CLV_PATH_VAR)
    set("${CLV_PATH_VAR}" "${LIB_PATH}" PARENT_SCOPE)
  endif()

  # Find the version header
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} "-print-file-name=${_version_header}"
    OUTPUT_VARIABLE HEADER_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)

  if("${HEADER_PATH}" STREQUAL "${_version_header}")
    return()
  endif()

  # Extract the version from the header
  set(SED "s/^\\s*#\\s*define\\s*${_version_macro}\\s*\"(\\S+)\"\\s*$/\\1/p; d")
  execute_process(
    COMMAND sed -E "${SED}" "${HEADER_PATH}"
    OUTPUT_VARIABLE VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(DEFINED CLV_VERSION_VAR)
    set("${CLV_VERSION_VAR}" "${VERSION}" PARENT_SCOPE)
  endif()
endfunction()
