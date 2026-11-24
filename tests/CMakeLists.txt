# tests/: Catch2 tests, run by ctest. Every .cpp here is a source of tee-tests (adding a file edits no CMake);
# tests/support/ holds what tests share (tee::test::nullDepth and the rest), headers only.
#
# The tests find the simulator and the content in this checkout (compiled in), and write what they keep under out/.
include(Catch)
file(GLOB _tee_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests ${_tee_test_sources})
tee_target_defaults(tee-tests)
target_include_directories(tee-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
catch_discover_tests(tee-tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 900)
