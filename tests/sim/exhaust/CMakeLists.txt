# tests/sim/exhaust/: card G's tests of the exhaust builder (sim/exhaust/), on its own harness (harness.fm, with the
# reference's exhaust copied alone in refexhaust.fm) through the host's JIT. ctest -R '^exhaust\.' runs them alone.
if(TARGET tee-tests-exhaust)
  return()
endif()
file(GLOB _tee_exhaust_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-exhaust ${_tee_exhaust_sources})
tee_target_defaults(tee-tests-exhaust)
target_include_directories(tee-tests-exhaust PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-exhaust PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-exhaust PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
# One compile a process (01-rules.md section 3): the fast cases run as two ctest entries, so each program compiles once:
# exhaust.engine (the [engine] cases, on EngineTest.cpp's one kept player, reset or rendered afresh at every case's
# start) and exhaust.harness (the rest, each case on a fresh Rig instance of the one compiled harness, Rig.h). Proved
# once by running each group's cases in reverse order. The nightly forms are discovered one by one.
catch_discover_tests(tee-tests-exhaust DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "exhaust."
  TEST_SPEC "[nightly]" PROPERTIES TIMEOUT 3600)
add_test(NAME exhaust.engine COMMAND tee-tests-exhaust "[engine]~[nightly]")
set_tests_properties(exhaust.engine PROPERTIES TIMEOUT 900 LABELS "exhaust;engine")
add_test(NAME exhaust.harness COMMAND tee-tests-exhaust "~[engine]~[nightly]")
set_tests_properties(exhaust.harness PROPERTIES TIMEOUT 900 LABELS "exhaust;harness")
