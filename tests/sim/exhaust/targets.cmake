# tests/sim/exhaust/targets.cmake: tee-exhaust-tests, card G's tests of the exhaust builder (sim/exhaust/) on its own
# harness (harness.fm) through the host's JIT; ctest runs each case as sim.exhaust.<name>. Included by this
# directory's CMakeLists.txt, or by hook.cmake.
include(Catch)
file(GLOB _tee_exhaust_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_executable(tee-exhaust-tests ${_tee_exhaust_sources})
tee_target_defaults(tee-exhaust-tests)
target_include_directories(tee-exhaust-tests PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-exhaust-tests PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-exhaust-tests PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-exhaust-tests DISCOVERY_MODE PRE_TEST TEST_PREFIX "sim.exhaust." PROPERTIES TIMEOUT 900 LABELS exhaust)
