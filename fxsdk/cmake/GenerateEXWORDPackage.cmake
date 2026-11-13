# Generate installable EX-word folders alongside a .d01 add-in.
# Usage: generate_exword_package(TARGET app D01 path APPID ABCDE
#          TITLE "App name" HTML_DIR html OUTPUT_DIR packages LOCALES ja cn)
set(_FXSDK_EXWORD_PACKAGE_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/PackageEXWORD.cmake")

function(generate_exword_package)
  cmake_parse_arguments(PKG "" "TARGET;D01;APPID;TITLE;HTML_DIR;OUTPUT_DIR" "LOCALES" ${ARGN})
  if(PKG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "generate_exword_package: Unexpected arguments: ${PKG_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT PKG_TARGET OR NOT TARGET "${PKG_TARGET}")
    message(FATAL_ERROR "generate_exword_package: TARGET must be an existing executable")
  endif()
  if(NOT PKG_D01 OR NOT PKG_HTML_DIR OR NOT PKG_APPID OR NOT DEFINED PKG_TITLE)
    message(FATAL_ERROR "generate_exword_package: D01, HTML_DIR, APPID and TITLE are required")
  endif()
  if(NOT PKG_APPID MATCHES "^[A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9]$")
    message(FATAL_ERROR "generate_exword_package: APPID must be five uppercase letters/digits")
  endif()
  if(NOT PKG_LOCALES)
    set(PKG_LOCALES ja cn)
  endif()
  if(NOT PKG_OUTPUT_DIR)
    set(PKG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/packages")
  endif()
  get_filename_component(PKG_D01 "${PKG_D01}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  get_filename_component(PKG_HTML_DIR "${PKG_HTML_DIR}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  get_filename_component(PKG_OUTPUT_DIR "${PKG_OUTPUT_DIR}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  get_filename_component(PKG_MODULE "${PKG_D01}" NAME)

  foreach(LOCALE IN LISTS PKG_LOCALES)
    file(GLOB PKG_HTML CONFIGURE_DEPENDS "${PKG_HTML_DIR}/${LOCALE}/*.htm")
    if(NOT PKG_HTML)
      message(FATAL_ERROR "generate_exword_package: No .htm templates for ${LOCALE}")
    endif()
    add_custom_target("${PKG_TARGET}_package_${LOCALE}" ALL
      COMMAND "${CMAKE_COMMAND}"
        "-DPKG_SOURCE_D01=${PKG_D01}"
        "-DPKG_HTML_DIR=${PKG_HTML_DIR}/${LOCALE}"
        "-DPKG_DEST=${PKG_OUTPUT_DIR}/${LOCALE}/${PKG_APPID}"
        "-DPKG_APPID=${PKG_APPID}"
        "-DPKG_MODULE=${PKG_MODULE}"
        "-DPKG_TITLE=${PKG_TITLE}"
        -P "${_FXSDK_EXWORD_PACKAGE_SCRIPT}"
      DEPENDS "${PKG_TARGET}"
      VERBATIM
      COMMENT "Packaging EX-word ${LOCALE}/${PKG_APPID}")
  endforeach()
endfunction()
