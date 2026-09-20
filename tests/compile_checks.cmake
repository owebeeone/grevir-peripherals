file(GLOB_RECURSE public_headers CONFIGURE_DEPENDS RELATIVE
  "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/src/*.hpp"
  "${PROJECT_SOURCE_DIR}/src/*.h")
set(header_sources)
foreach(header IN LISTS public_headers)
  string(MAKE_C_IDENTIFIER "${header}" identifier)
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${identifier}.cpp")
  file(WRITE "${source}" "#include <${header}>\n")
  list(APPEND header_sources "${source}")
endforeach()
add_library(grevir_peripherals_compile OBJECT native_compile.cpp timer_config_static_tests.cpp ${header_sources})
target_link_libraries(grevir_peripherals_compile PRIVATE grevir::peripherals)
set_target_properties(grevir_peripherals_compile PROPERTIES CXX_EXTENSIONS OFF)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
  message(FATAL_ERROR "Peripheral conflict probes currently require a Clang/GNU driver")
endif()
add_custom_target(grevir_peripherals_claim_checks ALL
  COMMAND "${CMAKE_COMMAND}"
    "-DCXX=${CMAKE_CXX_COMPILER}"
    "-DINCLUDE_DIRS=$<TARGET_PROPERTY:grevir_peripherals_compile,INCLUDE_DIRECTORIES>"
    "-DCASE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/pin_claim_probe.cpp"
    "-DLOG_DIR=${CMAKE_CURRENT_BINARY_DIR}/claim-results"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_pin_claims.cmake"
  COMMENT "Checking resource claims on the extracted GPIO wrappers"
  VERBATIM)

add_custom_target(grevir_peripherals_pwm_claim_checks ALL
  COMMAND "${CMAKE_COMMAND}"
    "-DCXX=${CMAKE_CXX_COMPILER}"
    "-DINCLUDE_DIRS=$<TARGET_PROPERTY:grevir_peripherals_compile,INCLUDE_DIRECTORIES>"
    "-DCASE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/pwm_claim_probe.cpp"
    "-DLOG_DIR=${CMAKE_CURRENT_BINARY_DIR}/claim-results"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_pwm_claims.cmake"
  COMMENT "Checking combined PWM pin and timer claims"
  VERBATIM)

add_custom_target(grevir_peripherals_storage_timer_checks ALL
  COMMAND "${CMAKE_COMMAND}"
    "-DCXX=${CMAKE_CXX_COMPILER}"
    "-DINCLUDE_DIRS=$<TARGET_PROPERTY:grevir_peripherals_compile,INCLUDE_DIRECTORIES>"
    "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DLOG_DIR=${CMAKE_CURRENT_BINARY_DIR}/claim-results"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_storage_timer_contracts.cmake"
  COMMENT "Checking storage regions and backend timer requirements"
  VERBATIM)
