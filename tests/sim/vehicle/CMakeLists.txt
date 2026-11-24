# tests/sim/vehicle/: card I's tests (the vehicle, sim/vehicle/), as tee-tests-vehicle; ctest -R '^vehicle\.' runs them.
# Every .cpp here is a source (adding one edits no CMake).
if(TARGET tee-tests-vehicle)
  return()
endif()
file(GLOB _tee_vehicle_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-vehicle ${_tee_vehicle_sources})
tee_target_defaults(tee-tests-vehicle)
target_include_directories(tee-tests-vehicle PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-vehicle PRIVATE Tee::host Tee::test Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_compile_definitions(tee-tests-vehicle PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-vehicle DISCOVERY_MODE PRE_TEST TEST_PREFIX "vehicle." PROPERTIES TIMEOUT 900)
