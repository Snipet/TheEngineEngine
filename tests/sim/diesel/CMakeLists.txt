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
catch_discover_tests(tee-tests-diesel DISCOVERY_MODE PRE_TEST TEST_PREFIX "diesel." PROPERTIES TIMEOUT 900)
