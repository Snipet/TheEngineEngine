# tests/host/: the host's and the tools' tests (card C): tee-host-tests, Catch2, run by ctest. Every .cpp here is a
# source (adding one edits no CMake). fixtures/standin/ is a small FMinus program with the simulator's outside
# (02-contracts.md, section 1), on which SimHost, Build and the rest are tested whatever state sim/ is in.
file(GLOB _tee_host_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-host-tests ${_tee_host_test_sources})
tee_target_defaults(tee-host-tests)
target_include_directories(tee-host-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-host-tests PRIVATE Tee::host Tee::test Catch2::Catch2WithMain)
target_compile_definitions(tee-host-tests PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_STANDIN_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/standin\""
  "TEE_HOST_TESTS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
catch_discover_tests(tee-host-tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 900)
# the tools a test runs, beside the tests in bin/
target_compile_definitions(tee-host-tests PRIVATE "TEE_BIN_DIR=\"$<TARGET_FILE_DIR:tee-host-tests>\"")
if(TARGET tee-dyno)
  add_dependencies(tee-host-tests tee-dyno)
endif()
