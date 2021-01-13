# Determine platform Code
if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  set(PC cg)
  set(INTF_DEFN FXCG50)
  set(INTF_LINK "-T;fxcg50.ld")
elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fx9860G)
  set(PC fx)
  set(INTF_DEFN FX9860G)
  set(INTF_LINK "-T;fx9860g.ld")
else()
  message(FATAL_ERROR "gint: unknown fxSDK platform '${FXSDK_PLATFORM}'")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} -print-file-name=libgint-${PC}.a
  OUTPUT_VARIABLE GINT_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${CMAKE_C_COMPILER} -print-file-name=include/gint/config.h
  OUTPUT_VARIABLE GINT_CONFIG_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(GINT_CONFIG_PATH)
  execute_process(
    COMMAND sed -E "s/#define.*GINT_VERSION\\s+\"(\\S+)\"$/\\1/p; d" ${GINT_CONFIG_PATH}
    OUTPUT_VARIABLE GINT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

if("${GINT_PATH}" STREQUAL "libgint-${PC}.a")
  set(COMPILER_HAS_LIBRARY FALSE)
else()
  set(COMPILER_HAS_LIBRARY TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gint
  REQUIRED_VARS COMPILER_HAS_LIBRARY
  VERSION_VAR GINT_VERSION
)

if(Gint_FOUND)
  if(NOT TARGET Gint::Gint)
    add_library(Gint::Gint UNKNOWN IMPORTED)
  endif()
  # No include directory or library path to specify, GCC handles that
  set_target_properties(Gint::Gint PROPERTIES
    IMPORTED_LOCATION "${GINT_PATH}"
    INTERFACE_COMPILE_OPTIONS -fstrict-volatile-bitfields
    INTERFACE_COMPILE_DEFINITIONS "${INTF_DEFN}"
    INTERFACE_LINK_LIBRARIES -lgcc
    INTERFACE_LINK_OPTIONS "${INTF_LINK}"
  )
endif()
