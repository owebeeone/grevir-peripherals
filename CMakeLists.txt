cmake_minimum_required(VERSION 3.20)
project(grevir_peripherals VERSION 0.1.0 LANGUAGES CXX)

if(NOT TARGET grevir::base)
  find_package(grevir-base CONFIG REQUIRED)
endif()

foreach(dependency IN ITEMS time core)
  if(NOT TARGET grevir::${dependency})
    find_package(grevir-${dependency} CONFIG REQUIRED)
  endif()
endforeach()

add_library(grevir_peripherals INTERFACE)
add_library(grevir::peripherals ALIAS grevir_peripherals)
set_target_properties(grevir_peripherals PROPERTIES EXPORT_NAME peripherals)
target_compile_features(grevir_peripherals INTERFACE cxx_std_23)
target_link_libraries(grevir_peripherals INTERFACE grevir::base grevir::time grevir::core)
target_include_directories(grevir_peripherals INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>)

include(GNUInstallDirs)
install(TARGETS grevir_peripherals EXPORT GrevirPeripheralsTargets)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING
  PATTERN "*.hpp" PATTERN "*.h")
install(EXPORT GrevirPeripheralsTargets NAMESPACE grevir::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/grevir-peripherals)
install(FILES cmake/grevir-peripherals-config.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/grevir-peripherals)
install(FILES LICENSE.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/grevir-peripherals)

option(GREVIR_BUILD_COMPILE_CHECKS "Compile native headers and template users" OFF)
option(GREVIR_BUILD_HOST_TESTS "Run deterministic native behavior tests" OFF)
if(GREVIR_BUILD_HOST_TESTS)
  enable_testing()
endif()
if(GREVIR_BUILD_COMPILE_CHECKS OR GREVIR_BUILD_HOST_TESTS)
  add_subdirectory(tests)
endif()
