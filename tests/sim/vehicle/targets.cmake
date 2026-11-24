# tests/sim/vehicle/targets.cmake: tee-vehicle-tests, card I's tests (the vehicle, sim/vehicle/), run by ctest under
# the names vehicle.*. Every .cpp beside this file is a source (adding one edits no CMake). Included by this
# directory's CMakeLists.txt, or by hook.cmake.
if(TARGET tee-vehicle-tests OR NOT TARGET Tee::host)
  return()
endif()
set(_tee_vehicle_dir "${CMAKE_CURRENT_LIST_DIR}")
include(Catch)
file(GLOB _tee_vehicle_sources CONFIGURE_DEPENDS "${_tee_vehicle_dir}/*.cpp")
add_executable(tee-vehicle-tests ${_tee_vehicle_sources})
tee_target_defaults(tee-vehicle-tests)
target_include_directories(tee-vehicle-tests PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${_tee_vehicle_dir}")
target_link_libraries(tee-vehicle-tests PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-vehicle-tests PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
catch_discover_tests(tee-vehicle-tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 900)
