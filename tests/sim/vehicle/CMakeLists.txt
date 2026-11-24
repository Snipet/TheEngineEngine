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
# The light cases one a process; the launches and the tyres' start (the whole simulator through tests/sim/ecu/Sim.h) in one process, one
# compile, the kept player reset between them (docs/plan/01-rules.md section 3).
catch_discover_tests(tee-tests-vehicle DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_SPEC "~[launch]~[sim]~[engine]" TEST_PREFIX "vehicle."
                     PROPERTIES TIMEOUT 900)
add_test(NAME "vehicle.launch" COMMAND tee-tests-vehicle "[launch]~[nightly],[sim]~[nightly]" --durations yes)
set_tests_properties("vehicle.launch" PROPERTIES TIMEOUT 900 LABELS "vehicle;launch")
# the cases through the host's tee::test::Scenario ([engine]): one process, so tee::test::compiled's kept programs (one
# unchecked, one with the checks) serve them all: two compiles in all
add_test(NAME "vehicle.engine" COMMAND tee-tests-vehicle "[engine]~[nightly]" --durations yes)
set_tests_properties("vehicle.engine" PROPERTIES TIMEOUT 900 LABELS "vehicle;engine")
