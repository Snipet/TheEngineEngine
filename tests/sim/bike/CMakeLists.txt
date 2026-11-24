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
# The light cases one a process, as discovered; the cases that compile the simulator (tagged [sim]) together in one
# entry, so sim/ is compiled once for all of them (01-rules.md section 3: one compile a process). Each of those cases
# plays its own Scenario through a fresh host, so none starts from what another left.
catch_discover_tests(tee-tests-bike DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_SPEC "~[sim]" TEST_PREFIX "bike."
                     PROPERTIES TIMEOUT 900)
add_test(NAME bike.sim COMMAND tee-tests-bike "[sim]")
set_tests_properties(bike.sim PROPERTIES TIMEOUT 900 LABELS "bike;sim;engine")
