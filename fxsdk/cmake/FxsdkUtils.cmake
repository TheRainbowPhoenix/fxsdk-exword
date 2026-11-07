# Who doesn't love to deal with escaping?!
function(shell_escape _str _out)
  string(REPLACE "'" "\\'" _str "${_str}")
  string(REPLACE "(" "\\(" _str "${_str}")
  string(REPLACE ")" "\\)" _str "${_str}")
  set("${_out}" "${_str}" PARENT_SCOPE)
endfunction()

# Add linker options supported by the selected GNU ld version.
function(fxsdk_add_rwx_warning_suppression)
  execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-prog-name=ld
    OUTPUT_VARIABLE FXSDK_LD
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE FXSDK_LD_RESULT
    ERROR_QUIET
  )

  if(FXSDK_LD_RESULT EQUAL 0 AND FXSDK_LD)
    execute_process(
      COMMAND "${FXSDK_LD}" --version
      OUTPUT_VARIABLE FXSDK_LD_VERSION_OUTPUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE FXSDK_LD_VERSION_RESULT
      ERROR_QUIET
    )

    if(FXSDK_LD_VERSION_RESULT EQUAL 0 AND
       FXSDK_LD_VERSION_OUTPUT MATCHES "GNU ld.* ([0-9]+)\\.([0-9]+)")
      if(CMAKE_MATCH_1 GREATER 2 OR
         (CMAKE_MATCH_1 EQUAL 2 AND CMAKE_MATCH_2 GREATER_EQUAL 39))
        add_link_options(-Wl,--no-warn-rwx-segments)
      endif()
    endif()
  endif()
endfunction()
