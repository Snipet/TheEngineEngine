# tests/sim/diesel/: card D2's tests (the heavy diesel: sim/combustion/heavy.fm, sim/vehicle/truck files): tee-tests-diesel, run by ctest as diesel.*.
# Reached by tests/CMakeLists.txt's glob of tests/sim/*/. Every .cpp here is a source (adding one edits no CMake).
if(TARGET tee-tests-diesel)
  return()
endif()
include(Catch)
file(GLOB _tee_diesel_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-diesel ${_tee_diesel_sources})
tee_target_defaults(tee-tests-diesel)
target_include_directories(tee-tests-diesel PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-diesel PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-diesel PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
# The fast tier as five grouped entries, each one process: the program compiles once a process (6.6 s) and the cases
# share it (Diesel.h's kept player, reset per case), so a group costs far less than its cases apart (01-rules.md §3:
# a group up to 90 s). The full forms, [nightly], one entry a case, labelled nightly (left out of the dev runs).
foreach(_group IN ITEMS
    "brakes|[brake]~[nightly]"
    "governor-injection|[governor],[injection]"
    "builds-cold-soak|[builds]~[nightly],[cold]~[nightly],[soak]"
    "truck|[truck]~[nightly],[truckparts]"
    "turbo|[turbo]~[brake]~[nightly]")
  string(REPLACE "|" ";" _pair "${_group}")
  list(GET _pair 0 _name)
  list(GET _pair 1 _spec)
  add_test(NAME "diesel.${_name}" COMMAND tee-tests-diesel "${_spec}")
  set_tests_properties("diesel.${_name}" PROPERTIES TIMEOUT 900 LABELS "diesel")
endforeach()
catch_discover_tests(tee-tests-diesel TEST_SPEC "[nightly]" DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "diesel."
                     PROPERTIES TIMEOUT 900)
