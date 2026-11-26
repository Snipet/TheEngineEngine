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
# The cases tagged [sim] compile the real simulator (seconds each; tee::test::compiled() keeps one per process): they
# run as a few tests, one process each, sharing the compiles; ctest would run each discovered case in a process of its
# own, a compile each. The default run's are two entries (the bakes, the rest), each under the 90 s a grouped entry may
# take; the [nightly] ones one. The rest are discovered one by one.
add_test(NAME host.sim.bakes COMMAND tee-host-tests "[sim][bake]~[nightly]" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(host.sim.bakes PROPERTIES TIMEOUT 900 LABELS "host;sim")
add_test(NAME host.sim.cases COMMAND tee-host-tests "[sim]~[bake]~[nightly]" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(host.sim.cases PROPERTIES TIMEOUT 900 LABELS "host;sim")
add_test(NAME host.sim.nightly COMMAND tee-host-tests "[sim][nightly]" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(host.sim.nightly PROPERTIES TIMEOUT 3600 LABELS "host;sim;nightly")
# a case tagged [timing] checks a wall-clock budget: alone, not beside a parallel run's load
add_test(NAME host.timing COMMAND tee-host-tests "[timing]~[sim]" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(host.timing PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE LABELS "host;timing")
catch_discover_tests(tee-host-tests DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_SPEC "~[sim]~[timing]" PROPERTIES TIMEOUT 900)
# the tools a test runs, beside the tests in bin/
target_compile_definitions(tee-host-tests PRIVATE "TEE_BIN_DIR=\"$<TARGET_FILE_DIR:tee-host-tests>\"")
foreach(_tool tee-dyno tee-render)
  if(TARGET ${_tool})
    add_dependencies(tee-host-tests ${_tool})
  endif()
endforeach()
