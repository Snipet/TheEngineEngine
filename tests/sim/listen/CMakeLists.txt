# tests/sim/listen/: package L's tests of the listener (sim/listen/): standalone on its rig (rig.fm) through the host's
# JIT, and in the whole simulator through tee::EnginePlayer. ctest -R '^listen\.' runs them alone.
if(TARGET tee-tests-listen)
  return()
endif()
include(Catch)
file(GLOB _tee_listen_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-listen ${_tee_listen_sources})
tee_target_defaults(tee-tests-listen)
target_include_directories(tee-tests-listen PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-listen PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-listen PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
# The default run's whole-simulator cases (the Catch tag [sim]) run as one ctest entry, listen.sim, so sim/ compiles once a
# process (some 6.6 CPU-s; 01-rules.md section 3: a grouped entry up to 90 s, each case under 30), on core's Bench's one
# kept player, reset for each Bench to a fresh one's state (shown by running them in declared and random order: the
# commit says so). The rig's light cases and the nightly ones are discovered one by one. listen.sim-tags fails if a
# default-run [engine] case is not in [sim].
catch_discover_tests(tee-tests-listen DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "listen."
  TEST_SPEC "~[sim]" PROPERTIES TIMEOUT 900 LABELS listen)
add_test(NAME listen.sim COMMAND tee-tests-listen "[sim]")
set_tests_properties(listen.sim PROPERTIES LABELS "listen;engine;sim" TIMEOUT 900)
add_test(NAME listen.sim-tags COMMAND tee-tests-listen --list-tests "[engine]~[sim]~[nightly]~[.]")
set_tests_properties(listen.sim-tags PROPERTIES LABELS listen TIMEOUT 60 PASS_REGULAR_EXPRESSION "[^0-9]0 matching test cases")
