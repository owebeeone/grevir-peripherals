if(GREVIR_BUILD_COMPILE_CHECKS)
  include(compile_checks.cmake)
endif()
if(GREVIR_BUILD_HOST_TESTS)
  if(NOT COMMAND catch_discover_tests)
    find_package(grevir-test-support CONFIG REQUIRED)
  endif()
  add_executable(grevir_peripherals_runtime gpio_test.cpp time_poller_test.cpp application_test.cpp
    debounce_test.cpp button_events_test.cpp pwm_test.cpp storage_test.cpp timer_config_static_tests.cpp)
  target_link_libraries(grevir_peripherals_runtime PRIVATE grevir::peripherals Catch2::Catch2WithMain)
  set_target_properties(grevir_peripherals_runtime PROPERTIES CXX_EXTENSIONS OFF)
  catch_discover_tests(grevir_peripherals_runtime TEST_PREFIX "peripherals."
    PROPERTIES LABELS "peripherals-mock" TIMEOUT 10)
endif()
