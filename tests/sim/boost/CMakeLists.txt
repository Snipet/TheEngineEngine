# tests/sim/boost/: card F's tests (the forced induction, sim/boost/): tee-tests-boost, run by ctest as boost.*.
# Reached by tests/CMakeLists.txt's glob of tests/sim/*/. Every .cpp here is a source (adding one edits no CMake).
if(TARGET tee-tests-boost)
  return()
endif()
file(GLOB _tee_boost_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-boost ${_tee_boost_sources})
tee_target_defaults(tee-tests-boost)
target_include_directories(tee-tests-boost PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-boost PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-boost PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
# The whole-engine cases ([engine], EngineTest.cpp's Engine) in one process, one compile of sim/, the kept player reset
# between them (docs/plan/01-rules.md section 3). The light cases (the rig's, each compiling the rig once a process)
# grouped by tag into three processes of some 5 to 17 s each here (CI reads boost about 1.4 times this machine; the 30 s
# rule): one process a case cost some 30 compiles (the steward's trim, 26 Nov: boost 84 of 80 test-s on CI). The third
# group is every light case the first two do not take, so a new case's tag can never leave it unrun. The nightly
# cases outside [engine] each its own entry by discovery, their tags their labels (lint rule 9), as before.
set(_tee_boost_light "~[engine]" "~[nightly]")
add_test(NAME "boost.rig.charger-map" COMMAND tee-tests-boost "[charger],[map],[cost],[nitrous],[passive]" ${_tee_boost_light} --durations yes)
set_tests_properties("boost.rig.charger-map" PROPERTIES TIMEOUT 900 LABELS "boost;charger;map;cost;nitrous;passive")
add_test(NAME "boost.rig.layout-turbo" COMMAND tee-tests-boost "[layout],[turbo],[intercooler]" ${_tee_boost_light} --durations yes)
set_tests_properties("boost.rig.layout-turbo" PROPERTIES TIMEOUT 900 LABELS "boost;layout;turbo;intercooler")
add_test(NAME "boost.rig.rest" COMMAND tee-tests-boost ${_tee_boost_light} "~[charger]" "~[map]" "~[cost]" "~[nitrous]" "~[passive]"
         "~[layout]" "~[turbo]" "~[intercooler]" --durations yes)
set_tests_properties("boost.rig.rest" PROPERTIES TIMEOUT 900 LABELS "boost;surge;rest;soak;scroll")
include(Catch)
catch_discover_tests(tee-tests-boost DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_SPEC "[nightly]~[engine]" TEST_PREFIX "boost."
                     PROPERTIES TIMEOUT 3600)
add_test(NAME "boost.engine" COMMAND tee-tests-boost "[engine]~[nightly]" --durations yes)
set_tests_properties("boost.engine" PROPERTIES TIMEOUT 900 LABELS "boost;engine")
