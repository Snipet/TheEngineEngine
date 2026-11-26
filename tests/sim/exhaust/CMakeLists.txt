# tests/sim/exhaust/: card G's tests of the exhaust builder (sim/exhaust/), on its own harness (harness.fm, with the
# reference's exhaust copied alone in refexhaust.fm) through the host's JIT. ctest -R '^exhaust\.' runs them alone.
if(TARGET tee-tests-exhaust)
  return()
endif()
file(GLOB _tee_exhaust_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-exhaust ${_tee_exhaust_sources})
tee_target_defaults(tee-tests-exhaust)
target_include_directories(tee-tests-exhaust PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-exhaust PRIVATE Tee::host Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_compile_definitions(tee-tests-exhaust PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
# One compile a process (01-rules.md section 3): the fast cases run as two ctest entries, so each program compiles once:
# exhaust.engine-1 and -2 (the [engine] cases, on EngineTest.cpp's one kept player, reset or rendered afresh at every case's
# start) and exhaust.harness (the rest, each case on a fresh Rig instance of the one compiled harness, Rig.h). Proved
# once by running each group's cases in reverse order. The nightly forms are discovered one by one.
catch_discover_tests(tee-tests-exhaust DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "exhaust."
  TEST_SPEC "[nightly]" PROPERTIES TIMEOUT 3600)
# (exhaust.engine in two entries, each one compile and its kept player, as boost.engine's 0510ef72: the one read 117 s on
# CI's reference runner at 04439bc4, over the 90 s an entry; part-1 the identity, the settle on request, the housing,
# the port's walls, the builds and the first block; part-2 the valves' flow, the bangs' books, the settle's landing, the
# dual X, the twin turbos, the drags and the random topologies. exhaust.engine-all fails if an [engine] case carries
# neither part's tag)
add_test(NAME exhaust.engine-1 COMMAND tee-tests-exhaust "[engine][part-1]~[nightly]" --durations yes)
add_test(NAME exhaust.engine-2 COMMAND tee-tests-exhaust "[engine][part-2]~[nightly]" --durations yes)
add_test(NAME exhaust.engine-all COMMAND tee-tests-exhaust "[engine]~[part-1]~[part-2]~[nightly]" --list-tests --verbosity quiet)
set_tests_properties(exhaust.engine-all PROPERTIES FAIL_REGULAR_EXPRESSION "." LABELS "exhaust;engine")
set_tests_properties(exhaust.engine-1 exhaust.engine-2 PROPERTIES TIMEOUT 900 LABELS "exhaust;engine")
add_test(NAME exhaust.harness COMMAND tee-tests-exhaust "~[engine]~[nightly]")
set_tests_properties(exhaust.harness PROPERTIES TIMEOUT 900 LABELS "exhaust;harness")
