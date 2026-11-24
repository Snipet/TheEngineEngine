# tests/sim/bike/: card B2's tests (the motorcycle, sim/vehicle/bike.fm), as tee-tests-bike; ctest -R '^bike\.' runs them.
# Every .cpp here is a source (adding one edits no CMake).
if(TARGET tee-tests-bike)
  return()
endif()
file(GLOB _tee_bike_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-bike ${_tee_bike_sources})
tee_target_defaults(tee-tests-bike)
target_include_directories(tee-tests-bike PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-bike PRIVATE Tee::host Tee::test Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_compile_definitions(tee-tests-bike PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-bike DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "bike." PROPERTIES TIMEOUT 900)
