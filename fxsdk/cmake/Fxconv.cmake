set(CMAKE_FXCONV_COMPILE_OBJECT
  "fxconv <SOURCE> -o <OBJECT> --toolchain=sh-elf --${FXSDK_PLATFORM}")

function(fxconv_declare_assets)
  cmake_parse_arguments(CONV "WITH_METADATA" "" "" ${ARGN})

  foreach(ASSET IN LISTS CONV_UNPARSED_ARGUMENTS)
    # Declare this source file as an FXCONV object
    set_source_files_properties("${ASSET}" PROPERTIES LANGUAGE FXCONV)

    # Set up a dependency to the local fxconv-metadata.txt
    if(DEFINED CONV_WITH_METADATA)
      get_filename_component(DIR "${ASSET}" DIRECTORY)
      set(METADATA "${DIR}/fxconv-metadata.txt")
      get_filename_component(METADATA "${METADATA}" ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
      set_source_files_properties("${ASSET}" PROPERTIES OBJECT_DEPENDS "${METADATA}")
    endif()
  endforeach()
endfunction()
